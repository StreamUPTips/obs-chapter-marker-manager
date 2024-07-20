#include "annotation-dock.hpp"
#include "chapter-marker-dock.hpp"
#include "streamup-record-chapter-manager.hpp"
#include "version.h"
#include <obs-data.h>
#include <obs-frontend-api.h>
#include <obs-module.h>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <qdesktopservices.h>
#include <QDialogButtonBox>
#include <QDir>
#include <QDockWidget>
#include <QFile>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QListWidget>
#include <QMainWindow>
#include <QMessageBox>
#include <QStyle>
#include <QTextStream>
#include <QVBoxLayout>

#define QT_TO_UTF8(str) str.toUtf8().constData()

extern void AddChapterMarkerHotkey(void *data, obs_hotkey_id id, obs_hotkey_t *hotkey, bool pressed);
QString currentChapterName;

//--------------------CONSTRUCTOR & DESTRUCTOR--------------------
ChapterMarkerDock::ChapterMarkerDock(QWidget *parent)
	: QFrame(parent),
	  annotationDock(nullptr),
	  exportChaptersToTextEnabled(false),
	  exportChaptersToXMLEnabled(false),
	  exportChaptersToFileEnabled(false),
	  insertChapterMarkersInVideoEnabled(false),
	  exportTextFilePath(""),
	  exportXMLFilePath(""),
	  defaultChapterName(obs_module_text("DefaultChapterName")),
	  ignoredScenes(),
	  chapterOnSceneChangeEnabled(false),
	  showPreviousChaptersEnabled(false),
	  addChapterSourceEnabled(false),
	  chapterCount(1),
	  settingsDialog(nullptr),
	  isFirstRunInRecording(true),
	  presetChapters(),
	  chapterHotkeys(),
	  presetChaptersDialog(nullptr),
	  presetChapterNameInput(nullptr),
	  addChapterButton(nullptr),
	  removeChapterButton(nullptr),
	  chaptersListWidget(nullptr),
	  exportChaptersToFileCheckbox(nullptr),
	  exportChaptersToTextCheckbox(nullptr),
	  exportChaptersToXMLCheckbox(nullptr),
	  exportSettingsGroup(nullptr),
	  insertChapterMarkersCheckbox(nullptr),
	  ignoredScenesDialog(nullptr),
	  sceneChangeSettingsGroup(nullptr),
	  chapterNameInput(new QLineEdit(this)),
	  settingsButton(new QPushButton(this)),
	  setIgnoredScenesButton(nullptr),
	  annotationButton(new QPushButton(this)),
	  setPresetChaptersButton(nullptr),
	  currentChapterTextLabel(new QLabel(obs_module_text("CurrentChapterLabel"), this)),
	  currentChapterNameLabel(new QLabel(obs_module_text("RecordingNotActive"), this)),
	  feedbackLabel(new QLabel("", this)),
	  previousChaptersList(new QListWidget(this)),
	  previousChaptersGroup(nullptr),
	  saveChapterMarkerButton(new QPushButton(obs_module_text("SaveChapterMarkerButton"), this)),
	  defaultChapterNameEdit(nullptr),
	  showPreviousChaptersCheckbox(nullptr),
	  addChapterSourceCheckbox(nullptr),
	  chapterOnSceneChangeCheckbox(nullptr),
	  ignoredScenesListWidget(nullptr),
	  ignoredScenesGroup(nullptr),
	  chapters(),
	  timestamps(),
	  textCheckboxLayout(nullptr),
	  xmlCheckboxLayout(nullptr),
	  exportSettingsLayout(nullptr)
{
	// UI Setup
	setupMainDockUI();
	// Signal Connections
	setupConnections();
	// OBS Event Callback
	setupOBSCallbacks();
	// initialise UI States
	refreshMainDockUI();
}

ChapterMarkerDock::~ChapterMarkerDock()
{
	for (const auto &chapterName : chapterHotkeys.keys()) {
		unregisterChapterHotkey(chapterName);
	}

	if (settingsDialog) {
		delete settingsDialog;
	}
	if (ignoredScenesDialog) {
		delete ignoredScenesDialog;
	}
}

//--------------------SIGNAL CONNECTIONS--------------------
void ChapterMarkerDock::setupConnections()
{
	// Websocket add annotation
	connect(this, &ChapterMarkerDock::addAnnotationSignal, this, &ChapterMarkerDock::onAddAnnotation);

	// Websocket add chapter
	connect(this, &ChapterMarkerDock::addChapterMarkerSignal, this, &ChapterMarkerDock::onAddChapterMarker);

	// Enter Chapter Name text field and button
	connect(chapterNameInput, &QLineEdit::returnPressed, saveChapterMarkerButton, &QPushButton::click);
	connect(saveChapterMarkerButton, &QPushButton::clicked, this, &ChapterMarkerDock::onAddChapterMarkerButton);

	// Settings button
	connect(settingsButton, &QPushButton::clicked, this, &ChapterMarkerDock::onSettingsClicked);

	// Annotation button
	connect(annotationButton, &QPushButton::clicked, this, &ChapterMarkerDock::onAnnotationClicked);

	// Previous Chapter list
	connect(previousChaptersList, &QListWidget::itemClicked, this, &ChapterMarkerDock::onPreviousChapterSelected);
	connect(previousChaptersList, &QListWidget::itemDoubleClicked, this, &ChapterMarkerDock::onPreviousChapterDoubleClicked);

	// Feedback label timer
	feedbackTimer.setInterval(5000);
	feedbackTimer.setSingleShot(true);
	connect(&feedbackTimer, &QTimer::timeout, [this]() { feedbackLabel->setText(""); });
}

void ChapterMarkerDock::setupOBSCallbacks()
{
	obs_frontend_add_event_callback(
		[](obs_frontend_event event, void *ptr) {
			auto dock = static_cast<ChapterMarkerDock *>(ptr);
			if (event == OBS_FRONTEND_EVENT_SCENE_CHANGED || event == OBS_FRONTEND_EVENT_SCENE_LIST_CHANGED) {
				dock->onSceneChanged();
			} else if (event == OBS_FRONTEND_EVENT_RECORDING_STOPPED) {
				dock->onRecordingStopped();
			}
		},
		this);
}

//--------------------MAIN DOCK UI--------------------
void ChapterMarkerDock::setupMainDockUI()
{
	// Set the frame style
	this->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);

	QVBoxLayout *mainDockLayout = new QVBoxLayout(this);
	mainDockLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	mainDockLayout->setSizeConstraint(QLayout::SetMinimumSize);

	setupMainDockCurrentChapterLayout(mainDockLayout);
	setupMainDockChapterInput(mainDockLayout);
	setupMainDockSaveButtonLayout(mainDockLayout);
	setupMainDockFeedbackLabel(mainDockLayout);
	setupMainDockPreviousChaptersGroup(mainDockLayout);

	setLayout(mainDockLayout);
}

void ChapterMarkerDock::setupMainDockCurrentChapterLayout(QVBoxLayout *mainLayout)
{
	QHBoxLayout *chapterLabelLayout = new QHBoxLayout();
	chapterLabelLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);

	currentChapterTextLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	currentChapterNameLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	currentChapterNameLabel->setWordWrap(true);
	currentChapterNameLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	currentChapterNameLabel->setProperty("themeID", "error");

	chapterLabelLayout->addWidget(currentChapterTextLabel);
	chapterLabelLayout->addWidget(currentChapterNameLabel);

	mainLayout->addLayout(chapterLabelLayout);
}

void ChapterMarkerDock::setupMainDockChapterInput(QVBoxLayout *mainLayout)
{
	chapterNameInput->setPlaceholderText(obs_module_text("EnterChapterName"));
	chapterNameInput->setToolTip(obs_module_text("EnterChapterNameTooltip"));
	mainLayout->addWidget(chapterNameInput);
}

void ChapterMarkerDock::setupMainDockSaveButtonLayout(QVBoxLayout *mainLayout)
{
	QHBoxLayout *saveButtonLayout = new QHBoxLayout();

	// Make Save Chapter Marker button fill the horizontal space
	saveChapterMarkerButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	saveChapterMarkerButton->setToolTip(obs_module_text("SaveChapterMarkerButtonTooltip"));

	// Configure the settings button
	settingsButton->setToolTip(obs_module_text("SettingsTooltip"));
	applyThemeIDToButton(settingsButton, "configIconSmall");

	// Configure the annotation button using the applyThemeIDToButton function
	annotationButton->setIcon(QIcon(":images/annotation-icon.svg"));
	annotationButton->setMinimumSize(32, 24);
	annotationButton->setMaximumSize(32, 24);
	annotationButton->setIconSize(QSize(20, 20));
	annotationButton->setToolTip(obs_module_text("AnnotationButtonTooltip"));

	// Add the Save Chapter Marker button and a stretch to push the settings button to the right
	saveButtonLayout->addWidget(saveChapterMarkerButton);
	saveButtonLayout->addStretch();
	saveButtonLayout->addWidget(annotationButton);
	saveButtonLayout->addWidget(settingsButton);

	saveButtonLayout->setAlignment(saveChapterMarkerButton, Qt::AlignLeft);
	saveButtonLayout->setAlignment(annotationButton, Qt::AlignRight);
	saveButtonLayout->setAlignment(settingsButton, Qt::AlignRight);

	mainLayout->addLayout(saveButtonLayout);
}

void ChapterMarkerDock::setupMainDockFeedbackLabel(QVBoxLayout *mainLayout)
{
	feedbackLabel->setProperty("themeID", "good");
	style()->polish(feedbackLabel);
	feedbackLabel->setWordWrap(true);
	mainLayout->addWidget(feedbackLabel);
}

void ChapterMarkerDock::setupMainDockPreviousChaptersGroup(QVBoxLayout *mainLayout)
{
	previousChaptersGroup = new QGroupBox(obs_module_text("PreviousChapters"), this);
	previousChaptersGroup->setStyleSheet(
		"QGroupBox { font-weight: bold; border: 1px solid gray; padding: 10px; margin-top: 10px; } QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top center; padding: 0 3px; }");
	previousChaptersGroup->setToolTip(obs_module_text("PreviousChaptersTooltip"));
	QVBoxLayout *previousChaptersLayout = new QVBoxLayout(previousChaptersGroup);
	previousChaptersLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft); // Align previous chapters layout to top left
	previousChaptersLayout->addWidget(previousChaptersList);
	previousChaptersGroup->setLayout(previousChaptersLayout);

	mainLayout->addWidget(previousChaptersGroup);
}

void ChapterMarkerDock::refreshMainDockUI()
{
	previousChaptersGroup->setVisible(showPreviousChaptersEnabled);
}

//--------------------MAIN DOCK UI EVENT HANDLERS--------------------
void ChapterMarkerDock::onAddChapterMarkerButton()
{
	if (!obs_frontend_recording_active()) {
		blog(LOG_WARNING, "[StreamUP Record Chapter Manager] Recording is not active. Chapter marker not added.");
		showFeedbackMessage(obs_module_text("ChapterMarkerNotActive"), true);
		return;
	}

	if (!exportChaptersToFileEnabled && !insertChapterMarkersInVideoEnabled) {
		showFeedbackMessage(obs_module_text("NoExportMethod"), true);
		return;
	}

	QString chapterName = getChapterName();
	if (chapterName.isEmpty()) {
		// Use the default chapter name if the user did not provide one
		if (useIncrementalChapterNames) {
			chapterName = defaultChapterName + " " + QString::number(chapterCount);
			chapterCount++;
		} else {
			chapterName = defaultChapterName;
		}
	}

	addChapterMarker(chapterName, obs_module_text("SourceManual"));
	chapterNameInput->clear();
}

void ChapterMarkerDock::onSettingsClicked()
{
	if (!settingsDialog) {
		settingsDialog = createSettingsUI();
	}
	settingsDialog->exec();
}

void ChapterMarkerDock::onRecordingStopped()
{
	if (!exportChaptersToFileEnabled && !insertChapterMarkersInVideoEnabled) {
		showFeedbackMessage(obs_module_text("NoExportMethod"), true);
		return;
	}

	currentChapterNameLabel->setText(obs_module_text("RecordingNotActive"));
	currentChapterNameLabel->setProperty("themeID", "error");
	currentChapterNameLabel->style()->unpolish(currentChapterNameLabel);
	currentChapterNameLabel->style()->polish(currentChapterNameLabel);

	showFeedbackMessage(obs_module_text("RecordingFinished"), false);

	QString timestamp = getCurrentRecordingTime();
	writeChapterToTextFile(obs_module_text("End"), timestamp, obs_module_text("Recording"));

	clearPreviousChaptersGroup();
	blog(LOG_INFO, "[StreamUP Record Chapter Manager] chapterCount: %d", chapterCount);

	incompatibleFileTypeMessageShown = false;
	chapterCount = 1; // Reset chapter count to 1
}

void ChapterMarkerDock::onPreviousChapterSelected()
{
	QListWidgetItem *currentItem = previousChaptersList->currentItem();
	if (currentItem) {
		chapterNameInput->setText(currentItem->text());
	}
}

void ChapterMarkerDock::onPreviousChapterDoubleClicked(QListWidgetItem *item)
{
	if (item) {
		chapterNameInput->setText(item->text());
		saveChapterMarkerButton->click();
	}
}

void ChapterMarkerDock::clearPreviousChaptersGroup()
{
	previousChaptersList->clear();
	chapters.clear();
	timestamps.clear();
}

void ChapterMarkerDock::loadAnnotationDock()
{
	const auto main_window = static_cast<QMainWindow *>(obs_frontend_get_main_window());
	obs_frontend_push_ui_translation(obs_module_get_string);

	if (!annotationDock) {
		annotationDock = new AnnotationDock(this, main_window);

		const QString title = QString::fromUtf8(obs_module_text("StreamUPChapterAnnotations"));
		const auto name = "AnnotationDock";

#if LIBOBS_API_VER >= MAKE_SEMANTIC_VERSION(30, 0, 0)
		obs_frontend_add_dock_by_id(name, QT_TO_UTF8(title), annotationDock);
#else
		auto dock = new QDockWidget(main_window);
		dock->setObjectName(name);
		dock->setWindowTitle(title);
		dock->setWidget(annotationDock);
		dock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
		dock->setFloating(true);
		dock->hide();
		obs_frontend_add_dock(dock);
#endif
		obs_frontend_pop_ui_translation();
	}
}

void ChapterMarkerDock::onAnnotationClicked(bool refresh)
{
	if (!exportChaptersToFileEnabled && !refresh) {
		showFeedbackMessage(obs_module_text("AnnotationErrorExportNotActive"), true);
		return;
	}

	if (!refresh) {
		annotationDock->parentWidget()->show();
		annotationDock->parentWidget()->raise();
	}
}

void ChapterMarkerDock::setAnnotationDock(AnnotationDock *dock)
{
	annotationDock = dock;
}

//--------------------MAIN DOCK LABELS--------------------
void ChapterMarkerDock::updateCurrentChapterLabel(const QString &chapterName)
{
	if (!exportChaptersToFileEnabled && !insertChapterMarkersInVideoEnabled) {
		showFeedbackMessage(obs_module_text("NoExportMethod"), true);
		return;
	}

	QFontMetrics metrics(currentChapterNameLabel->font());
	int labelWidth = currentChapterNameLabel->width();

	QString elidedText = metrics.elidedText(chapterName, Qt::ElideRight, labelWidth);

	currentChapterNameLabel->setText(elidedText);

	currentChapterNameLabel->setProperty("themeID", "good");
	currentChapterNameLabel->style()->unpolish(currentChapterNameLabel);
	currentChapterNameLabel->style()->polish(currentChapterNameLabel);
}

void ChapterMarkerDock::showFeedbackMessage(const QString &message, bool isError)
{
	QFontMetrics metrics(feedbackLabel->font());
	int labelWidth = feedbackLabel->width();

	QString elidedText = metrics.elidedText(message, Qt::ElideRight, labelWidth);

	feedbackLabel->setWordWrap(true);
	feedbackLabel->setText(elidedText);

	if (isError) {
		setChapterMarkerFeedbackLabel(elidedText, "error");
	} else {
		setChapterMarkerFeedbackLabel(elidedText, "good");
	}

	feedbackTimer.start();
}

//--------------------SETTINGS UI--------------------
QDialog *ChapterMarkerDock::createSettingsUI()
{
	settingsDialog = new QDialog(this);
	settingsDialog->setWindowTitle(obs_module_text("ChapterMarkerManagerSettings"));

	QVBoxLayout *mainLayout = new QVBoxLayout(settingsDialog);

	setupSettingsGeneralGroup(mainLayout);
	setupSettingsExportGroup(mainLayout);
	setupSettingsAutoChapterGroup(mainLayout);

	// Create the label with HTML formatted text
	QString infoText =
		QString("<br>"
			"<a href='https://streamup.tips/product/obs-chapter-marker-manager'>StreamUP Chapter Marker Manager</a> • version %1"
			"<br>"
			"by <a href='https://doras.to/Andi'>Andi Stone (Andilippi)</a>")
			.arg(PROJECT_VERSION);

	QLabel *infoLabel = new QLabel(infoText, settingsDialog);
	infoLabel->setTextFormat(Qt::RichText);
	infoLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
	infoLabel->setOpenExternalLinks(true);
	infoLabel->setAlignment(Qt::AlignCenter);
	mainLayout->addWidget(infoLabel);

	// Create the 'Support my plugins' button
	QPushButton *supportButton = new QPushButton("❤️ Support My Plugins ❤️", settingsDialog);
	connect(supportButton, &QPushButton::clicked, this, []() { QDesktopServices::openUrl(QUrl("https://doras.to/Andi")); });

	// Create a layout for the label and button
	QVBoxLayout *infoLayout = new QVBoxLayout();
	infoLayout->addWidget(infoLabel);
	infoLayout->addWidget(supportButton);
	infoLayout->setAlignment(Qt::AlignCenter);

	// Add the info layout to the main layout
	mainLayout->addLayout(infoLayout);

	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, settingsDialog);
	connect(buttonBox, &QDialogButtonBox::accepted, this, &ChapterMarkerDock::saveSettingsAndCloseDialog);
	connect(buttonBox, &QDialogButtonBox::rejected, settingsDialog, &QDialog::reject);
	mainLayout->addWidget(buttonBox);

	settingsDialog->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
	settingsDialog->adjustSize();

	return settingsDialog;
}

void ChapterMarkerDock::setupSettingsGeneralGroup(QVBoxLayout *mainLayout)
{
	QGroupBox *generalSettingsGroup = new QGroupBox(obs_module_text("GeneralSettings"), settingsDialog);
	QVBoxLayout *generalSettingsLayout = new QVBoxLayout(generalSettingsGroup);

	QLabel *defaultChapterNameLabel = new QLabel(obs_module_text("GeneralSettingsDefaultChapterName"), generalSettingsGroup);
	defaultChapterNameLabel->setToolTip(obs_module_text("DefaultChapterNameTooltip"));

	defaultChapterNameEdit = new QLineEdit(generalSettingsGroup);
	defaultChapterNameEdit->setPlaceholderText(obs_module_text("GeneralSettingsDefaultChapterPlaceholder"));
	defaultChapterNameEdit->setToolTip(obs_module_text("DefaultChapterNameTooltip"));
	defaultChapterNameEdit->setText(defaultChapterName);
	defaultChapterNameEdit->setMinimumWidth(200);

	// Create a horizontal layout to place the label and text box side by side
	QHBoxLayout *chapterNameLayout = new QHBoxLayout;
	chapterNameLayout->addWidget(defaultChapterNameLabel);
	chapterNameLayout->addWidget(defaultChapterNameEdit);

	// Add the horizontal layout to the general settings layout
	generalSettingsLayout->addLayout(chapterNameLayout);

	QCheckBox *useIncrementalChapterNamesCheckbox =
		new QCheckBox(obs_module_text("UseIncrementalChapterNames"), generalSettingsGroup);
	useIncrementalChapterNamesCheckbox->setToolTip(obs_module_text("UseIncrementalChapterNamesTooltip"));
	generalSettingsLayout->addWidget(useIncrementalChapterNamesCheckbox);
	useIncrementalChapterNamesCheckbox->setChecked(useIncrementalChapterNames);
	connect(useIncrementalChapterNamesCheckbox, &QCheckBox::toggled, this,
		[this](bool checked) { useIncrementalChapterNames = checked; });

	showPreviousChaptersCheckbox = new QCheckBox(obs_module_text("GeneralSettingsShowChapterHistory"), generalSettingsGroup);
	showPreviousChaptersCheckbox->setToolTip(obs_module_text("PreviousChaptersTooltip"));
	generalSettingsLayout->addWidget(showPreviousChaptersCheckbox);
	showPreviousChaptersCheckbox->setChecked(showPreviousChaptersEnabled);

	addChapterSourceCheckbox = new QCheckBox(obs_module_text("GeneralSettingsAddChapterSource"), generalSettingsGroup);
	addChapterSourceCheckbox->setToolTip(obs_module_text("GeneralSettingsAddChapterSourceTooltip"));
	generalSettingsLayout->addWidget(addChapterSourceCheckbox);
	addChapterSourceCheckbox->setChecked(addChapterSourceEnabled);

	setPresetChaptersButton = new QPushButton(obs_module_text("GeneralSettingsSetPresetHotkeys"), generalSettingsGroup);
	setPresetChaptersButton->setToolTip(obs_module_text("GeneralSettingsSetPresetHotkeysTooltip"));
	connect(setPresetChaptersButton, &QPushButton::clicked, this, &ChapterMarkerDock::onSetPresetChaptersButtonClicked);
	generalSettingsLayout->addWidget(setPresetChaptersButton);

	generalSettingsGroup->setLayout(generalSettingsLayout);
	generalSettingsGroup->adjustSize();

	// Set the size policy to Preferred for width and Fixed for height
	generalSettingsGroup->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

	mainLayout->addWidget(generalSettingsGroup);
}

void ChapterMarkerDock::setupSettingsExportGroup(QVBoxLayout *mainLayout)
{
	exportSettingsGroup = new QGroupBox(obs_module_text("Export"));
	exportSettingsLayout = new QVBoxLayout(exportSettingsGroup);

	// Insert chapter markers into video file checkbox
	insertChapterMarkersCheckbox = new QCheckBox(obs_module_text("ExportSettingsInsertIntoFile"), exportSettingsGroup);
	insertChapterMarkersCheckbox->setToolTip(obs_module_text("ExportSettingsInsertIntoFileTooltip"));
	exportSettingsLayout->addWidget(insertChapterMarkersCheckbox);
	insertChapterMarkersCheckbox->setChecked(insertChapterMarkersInVideoEnabled);

	if (obs_get_version() < MAKE_SEMANTIC_VERSION(30, 2, 0)) {
		insertChapterMarkersCheckbox->setEnabled(false);
		insertChapterMarkersCheckbox->setChecked(false);
	}

	// Create check boxes
	exportChaptersToFileCheckbox = new QCheckBox(obs_module_text("ExportSettingsExportToFile"), exportSettingsGroup);
	exportChaptersToFileCheckbox->setToolTip(obs_module_text("ExportSettingsExportToFileTooltip"));
	exportChaptersToTextCheckbox = new QCheckBox(obs_module_text("ExportSettingsExportToText"), exportSettingsGroup);
	exportChaptersToTextCheckbox->setToolTip(obs_module_text("ExportSettingsExportToTextTooltip"));
	exportChaptersToXMLCheckbox = new QCheckBox(obs_module_text("ExportSettingsExportToXml"), exportSettingsGroup);
	exportChaptersToXMLCheckbox->setToolTip(obs_module_text("ExportSettingsExportToXmlTooltip"));

	// Set check boxes visually
	exportChaptersToFileCheckbox->setChecked(exportChaptersToFileEnabled);
	exportChaptersToTextCheckbox->setChecked(exportChaptersToTextEnabled);
	exportChaptersToXMLCheckbox->setChecked(exportChaptersToXMLEnabled);
	exportChaptersToTextCheckbox->setVisible(exportChaptersToFileEnabled);
	exportChaptersToXMLCheckbox->setVisible(exportChaptersToFileEnabled);

	connect(exportChaptersToFileCheckbox, &QCheckBox::toggled, this, &ChapterMarkerDock::onExportChaptersToFileToggled);

	exportSettingsLayout->addWidget(exportChaptersToFileCheckbox);

	textCheckboxLayout = new QHBoxLayout;
	xmlCheckboxLayout = new QHBoxLayout;
	// Add a spacer to the layouts to create the indent
	textCheckboxLayout->addSpacing(20);
	xmlCheckboxLayout->addSpacing(20);
	// Add the checkboxes to the layouts
	textCheckboxLayout->addWidget(exportChaptersToTextCheckbox);
	xmlCheckboxLayout->addWidget(exportChaptersToXMLCheckbox);

	// Add the QHBoxLayouts to the main layout
	if (!exportChaptersToFileEnabled) {
		exportSettingsLayout->removeItem(textCheckboxLayout);
		exportSettingsLayout->removeItem(xmlCheckboxLayout);
	} else {
		exportSettingsLayout->addLayout(textCheckboxLayout);
		exportSettingsLayout->addLayout(xmlCheckboxLayout);
	}

	exportSettingsGroup->setLayout(exportSettingsLayout);

	// Set the size policy to Preferred for width and Fixed for height
	exportSettingsGroup->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

	mainLayout->addWidget(exportSettingsGroup);
}

void ChapterMarkerDock::setupSettingsAutoChapterGroup(QVBoxLayout *mainLayout)
{
	sceneChangeSettingsGroup = new QGroupBox(obs_module_text("AutoChapterSettings"), settingsDialog);
	QVBoxLayout *sceneChangeSettingsLayout = new QVBoxLayout(sceneChangeSettingsGroup);

	// Add chapter on scene change checkbox
	chapterOnSceneChangeCheckbox = new QCheckBox(obs_module_text("AutoChapterOnSceneChange"), sceneChangeSettingsGroup);
	chapterOnSceneChangeCheckbox->setToolTip(obs_module_text("AutoChapterOnSceneChangeTooltip"));
	sceneChangeSettingsLayout->addWidget(chapterOnSceneChangeCheckbox);
	chapterOnSceneChangeCheckbox->setChecked(chapterOnSceneChangeEnabled);
	connect(chapterOnSceneChangeCheckbox, &QCheckBox::toggled, this, &ChapterMarkerDock::onChapterOnSceneChangeToggled);

	// Set ignore scene button
	setIgnoredScenesButton = new QPushButton(obs_module_text("AutoChapterSetIgnoredScenes"), sceneChangeSettingsGroup);
	setIgnoredScenesButton->setToolTip(obs_module_text("AutoChapterSetIgnoredScenesTooltip"));
	setIgnoredScenesButton->setVisible(chapterOnSceneChangeEnabled);
	connect(setIgnoredScenesButton, &QPushButton::clicked, this, &ChapterMarkerDock::onSetIgnoredScenesClicked);
	sceneChangeSettingsLayout->addWidget(setIgnoredScenesButton);

	sceneChangeSettingsGroup->setLayout(sceneChangeSettingsLayout);
	sceneChangeSettingsGroup->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

	mainLayout->addWidget(sceneChangeSettingsGroup);
}

void ChapterMarkerDock::saveSettingsAndCloseDialog()
{
	exportChaptersToFileEnabled = exportChaptersToFileCheckbox->isChecked();
	if (annotationDock) {
		annotationDock->updateInputState(exportChaptersToFileEnabled); // Update input state
	}
	SaveSettings();

	obs_data_t *settings = SaveLoadSettingsCallback(nullptr, false);

	if (settings) {
		LoadSettings(settings);
		obs_data_release(settings);
	}

	refreshMainDockUI();
	settingsDialog->accept();
}

//--------------------SETTINGS UI EVENT HANDLERS--------------------
void ChapterMarkerDock::onExportChaptersToFileToggled(bool checked)
{
	exportChaptersToFileEnabled = checked;
	exportChaptersToTextCheckbox->setVisible(checked);
	exportChaptersToXMLCheckbox->setVisible(checked);

	if (!checked) {
		exportSettingsLayout->removeItem(textCheckboxLayout);
		exportSettingsLayout->removeItem(xmlCheckboxLayout);
	} else {
		exportSettingsLayout->addLayout(textCheckboxLayout);
		exportSettingsLayout->addLayout(xmlCheckboxLayout);
	}

	QSize size = exportSettingsGroup->sizeHint();
	int newHeight = size.height();
	exportSettingsGroup->setFixedHeight(newHeight);

	settingsDialog->adjustSize();
}

void ChapterMarkerDock::onChapterOnSceneChangeToggled(bool checked)
{
	setIgnoredScenesButton->setVisible(checked);

	QSize size = sceneChangeSettingsGroup->sizeHint();
	int newHeight = size.height();
	sceneChangeSettingsGroup->setFixedHeight(newHeight);

	settingsDialog->adjustSize();
}

void ChapterMarkerDock::onSetIgnoredScenesClicked()
{
	if (!ignoredScenesDialog) {
		ignoredScenesDialog = createIgnoredScenesUI();
	}
	ignoredScenesDialog->exec();
}

void ChapterMarkerDock::onSetPresetChaptersButtonClicked()
{
	if (!presetChaptersDialog) {
		setupPresetChaptersDialog();
	}

	// Populate the chaptersListWidget with presetChapters
	chaptersListWidget->clear();
	for (const auto &chapter : presetChapters) {
		chaptersListWidget->addItem(chapter);
	}

	presetChaptersDialog->exec();
}

//--------------------PRESET CHAPTERS--------------------
void ChapterMarkerDock::setupPresetChaptersDialog()
{
	presetChaptersDialog = new QDialog(this);
	presetChaptersDialog->setWindowTitle(obs_module_text("GeneralSettingsSetPresetHotkeys"));

	QVBoxLayout *dialogLayout = new QVBoxLayout(presetChaptersDialog);

	// Explanation label
	QLabel *explanationLabel = new QLabel(obs_module_text("GeneralSettingsSetPresetChaptersExplanation"), presetChaptersDialog);
	explanationLabel->setWordWrap(true);
	dialogLayout->addWidget(explanationLabel);

	// Input field for new chapter names
	presetChapterNameInput = new QLineEdit(presetChaptersDialog);
	presetChapterNameInput->setPlaceholderText(obs_module_text("SetPresetHotkeysChapterNameInput"));
	presetChapterNameInput->setToolTip(obs_module_text("SetPresetHotkeysChapterNameInputTooltip"));
	dialogLayout->addWidget(presetChapterNameInput);

	// Add and Remove buttons
	QHBoxLayout *buttonsLayout = new QHBoxLayout();
	addChapterButton = new QPushButton(obs_module_text("SetPresetHotkeysAddChapter"), presetChaptersDialog);
	addChapterButton->setToolTip(obs_module_text("SetPresetHotkeysAddChapterTooltip"));
	removeChapterButton = new QPushButton(obs_module_text("SetPresetHotkeyRemoveChapter"), presetChaptersDialog);
	removeChapterButton->setToolTip(obs_module_text("SetPresetHotkeyRemoveChapterTooltip"));
	buttonsLayout->addWidget(addChapterButton);
	buttonsLayout->addWidget(removeChapterButton);
	dialogLayout->addLayout(buttonsLayout);

	// List widget to display chapters
	chaptersListWidget = new QListWidget(presetChaptersDialog);
	dialogLayout->addWidget(chaptersListWidget);

	// Connections for buttons
	connect(addChapterButton, &QPushButton::clicked, this, [this]() {
		QString chapterName = presetChapterNameInput->text().trimmed();
		if (!chapterName.isEmpty() && !presetChapters.contains(chapterName)) {
			presetChapters.append(chapterName);
			chaptersListWidget->addItem(chapterName);
			presetChapterNameInput->clear();
			registerChapterHotkey(chapterName);
		}
	});

	connect(removeChapterButton, &QPushButton::clicked, this, [this]() {
		QList<QListWidgetItem *> selectedItems = chaptersListWidget->selectedItems();
		for (QListWidgetItem *item : selectedItems) {
			QString chapterName = item->text();
			presetChapters.removeOne(chapterName);
			delete item;
			unregisterChapterHotkey(chapterName);
		}
	});

	// OK and Cancel buttons
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, presetChaptersDialog);
	connect(buttonBox, &QDialogButtonBox::accepted, presetChaptersDialog, &QDialog::accept);
	connect(buttonBox, &QDialogButtonBox::rejected, presetChaptersDialog, &QDialog::reject);
	dialogLayout->addWidget(buttonBox);

	presetChaptersDialog->setLayout(dialogLayout);
}

void ChapterMarkerDock::registerChapterHotkey(const QString &chapterName)
{
	if (!chapterHotkeys.contains(chapterName)) {
		obs_hotkey_id hotkeyId = obs_hotkey_register_frontend(QT_TO_UTF8(chapterName), QT_TO_UTF8(chapterName),
								      AddChapterMarkerHotkey, this);

		if (hotkeyId != OBS_INVALID_HOTKEY_ID) {
			chapterHotkeys.insert(chapterName, hotkeyId);
		}
	}
}

void ChapterMarkerDock::unregisterChapterHotkey(const QString &chapterName)
{
	if (chapterHotkeys.contains(chapterName)) {
		obs_hotkey_id hotkeyId = chapterHotkeys.value(chapterName);
		obs_hotkey_unregister(hotkeyId);
		chapterHotkeys.remove(chapterName);
	}
}

void ChapterMarkerDock::SaveChapterHotkeys(obs_data_t *settings)
{
	obs_data_array_t *hotkeysArray = obs_data_array_create();
	for (const auto &chapterName : chapterHotkeys.keys()) {
		obs_data_t *hotkeyData = obs_data_create();
		obs_data_set_string(hotkeyData, "chapterName", QT_TO_UTF8(chapterName));

		obs_data_array_t *hotkeySaveArray = obs_hotkey_save(chapterHotkeys[chapterName]);
		obs_data_set_array(hotkeyData, "hotkeyData", hotkeySaveArray);
		obs_data_array_release(hotkeySaveArray);

		obs_data_array_push_back(hotkeysArray, hotkeyData);
		obs_data_release(hotkeyData);
	}
	obs_data_set_array(settings, "chapterHotkeys", hotkeysArray);
	obs_data_array_release(hotkeysArray);
}

void ChapterMarkerDock::LoadChapterHotkeys(obs_data_t *settings)
{
	obs_data_array_t *hotkeysArray = obs_data_get_array(settings, "chapterHotkeys");
	if (!hotkeysArray)
		return;

	for (size_t i = 0; i < obs_data_array_count(hotkeysArray); ++i) {
		obs_data_t *hotkeyData = obs_data_array_item(hotkeysArray, i);
		const char *chapterName = obs_data_get_string(hotkeyData, "chapterName");
		obs_data_array_t *hotkeyLoadArray = obs_data_get_array(hotkeyData, "hotkeyData");

		if (chapterName && hotkeyLoadArray) {
			obs_hotkey_id hotkeyId =
				obs_hotkey_register_frontend(chapterName, chapterName, AddChapterMarkerHotkey, this);

			if (hotkeyId != OBS_INVALID_HOTKEY_ID) {
				obs_hotkey_load(hotkeyId, hotkeyLoadArray);
				chapterHotkeys.insert(QString::fromUtf8(chapterName), hotkeyId);
			}
			obs_data_array_release(hotkeyLoadArray);
		}
		obs_data_release(hotkeyData);
	}
	obs_data_array_release(hotkeysArray);
}

void ChapterMarkerDock::LoadPresetChapters(obs_data_t *settings)
{
	obs_data_array_t *chaptersArray = obs_data_get_array(settings, "presetChapters");
	if (chaptersArray) {
		for (size_t i = 0; i < obs_data_array_count(chaptersArray); ++i) {
			obs_data_t *chapterData = obs_data_array_item(chaptersArray, i);
			const char *chapterName = obs_data_get_string(chapterData, "chapterName");

			if (chapterName) {
				presetChapters.append(QString::fromUtf8(chapterName));
			}
			obs_data_release(chapterData);
		}
		obs_data_array_release(chaptersArray);
	}
}

//--------------------IGNORED SCENES UI--------------------
QDialog *ChapterMarkerDock::createIgnoredScenesUI()
{
	QDialog *dialog = new QDialog(this);
	dialog->setWindowTitle(obs_module_text("IgnoredScenes"));

	QVBoxLayout *mainLayout = new QVBoxLayout(dialog);

	ignoredScenesListWidget = new QListWidget(dialog);
	ignoredScenesListWidget->setSelectionMode(QAbstractItemView::MultiSelection);
	ignoredScenesListWidget->setToolTip(obs_module_text("IgnoredScenesTooltip"));

	// Disable horizontal scrolling
	ignoredScenesListWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	// Populate the list widget with OBS scene names
	populateIgnoredScenesListWidget();

	// Adjust the width of the list widget to fit the contents
	int longestNameWidth = ignoredScenesListWidget->sizeHintForColumn(0);
	ignoredScenesListWidget->setFixedWidth(longestNameWidth + 30); // Add some padding

	// Calculate the height for 10 entries
	int rowHeight = ignoredScenesListWidget->sizeHintForRow(0);
	int listHeight = rowHeight * 10;
	ignoredScenesListWidget->setFixedHeight(listHeight);

	mainLayout->addWidget(ignoredScenesListWidget);

	// Create OK and Cancel buttons
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, dialog);
	connect(buttonBox, &QDialogButtonBox::accepted, this, &ChapterMarkerDock::saveIgnoredScenes);
	connect(buttonBox, &QDialogButtonBox::rejected, dialog, &QDialog::reject);

	mainLayout->addWidget(buttonBox);

	dialog->setLayout(mainLayout);

	// Set the size policy to Preferred for width and Fixed for height
	dialog->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

	// Adjust the dialog size initially to fit the contents
	dialog->adjustSize();

	return dialog;
}

void ChapterMarkerDock::populateIgnoredScenesListWidget()
{
	ignoredScenesListWidget->clear();

	char **scene_names = obs_frontend_get_scene_names();
	if (scene_names) {
		for (char **name = scene_names; *name; name++) {
			QListWidgetItem *item = new QListWidgetItem(QString::fromUtf8(*name), ignoredScenesListWidget);
			// Check if this scene is in the ignoredScenes list and select it if it is
			if (ignoredScenes.contains(QString::fromUtf8(*name))) {
				item->setSelected(true);
			}
		}
		bfree(scene_names);
	}
}

void ChapterMarkerDock::saveIgnoredScenes()
{
	ignoredScenes.clear();
	QList<QListWidgetItem *> selectedItems = ignoredScenesListWidget->selectedItems();
	for (QListWidgetItem *item : selectedItems) {
		ignoredScenes << item->text();
	}

	// Save the settings
	SaveSettings();

	if (ignoredScenesDialog) {
		ignoredScenesDialog->accept();
	}
}

//--------------------FILE MANAGEMENT--------------------
void ChapterMarkerDock::createExportFiles()
{
	if (!exportChaptersToFileEnabled) {
		return;
	}

	obs_output_t *output = obs_frontend_get_recording_output();
	if (!output) {
		blog(LOG_ERROR, "[StreamUP Record Chapter Manager] Could not get the recording output.");
		return;
	}

	obs_data_t *settings = obs_output_get_settings(output);
	if (!settings) {
		blog(LOG_ERROR, "[StreamUP Record Chapter Manager] Could not get the recording output settings.");
		obs_output_release(output);
		return;
	}

	const char *recording_path = obs_data_get_string(settings, "path");
	if (!recording_path || strlen(recording_path) == 0) {
		blog(LOG_ERROR, "[StreamUP Record Chapter Manager] Could not get the recording output path.");
		obs_data_release(settings);
		obs_output_release(output);
		return;
	}

	QString outputPath = QString::fromUtf8(recording_path);
	obs_data_release(settings);
	obs_output_release(output);

	blog(LOG_INFO, "[StreamUP Record Chapter Manager] Recording path: %s", QT_TO_UTF8(outputPath));

	QFileInfo fileInfo(outputPath);
	QString baseName = fileInfo.completeBaseName();
	QString directoryPath = fileInfo.absolutePath();

	if (exportChaptersToTextEnabled) {
		QString chapterFilePath = directoryPath + "/" + baseName + "_chapters.txt";
		QFile file(chapterFilePath);
		if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
			QTextStream out(&file);
			out << "Chapter Markers for " << baseName << "\n";
			file.close();
			blog(LOG_INFO, "[StreamUP Record Chapter Manager] Created chapter file: %s", QT_TO_UTF8(chapterFilePath));
			setExportTextFilePath(chapterFilePath);
		} else {
			blog(LOG_ERROR, "[StreamUP Record Chapter Manager] Failed to create chapter file: %s",
			     QT_TO_UTF8(chapterFilePath));
		}
	}

	if (exportChaptersToXMLEnabled) {
		QString xmlFilePath = directoryPath + "/" + baseName + "_chapters.xml";
		QFile xmlFile(xmlFilePath);
		if (xmlFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
			QTextStream out(&xmlFile);
			out << "<ChapterMarkers>\n";
			out << "  <Title>" << baseName << "</Title>\n";
			out << "</ChapterMarkers>\n";
			xmlFile.close();
			blog(LOG_INFO, "[StreamUP Record Chapter Manager] Created XML chapter file: %s", QT_TO_UTF8(xmlFilePath));
			setExportXMLFilePath(xmlFilePath);
		} else {
			blog(LOG_ERROR, "[StreamUP Record Chapter Manager] Failed to create XML chapter file: %s",
			     QT_TO_UTF8(xmlFilePath));
		}
	}
}

void ChapterMarkerDock::setExportTextFilePath(const QString &filePath)
{
	exportTextFilePath = filePath;
}

void ChapterMarkerDock::writeChapterToTextFile(const QString &chapterName, const QString &timestamp, const QString &chapterSource)
{
	if (!exportChaptersToFileEnabled || !exportChaptersToTextEnabled) {
		return;
	}

	if (exportTextFilePath.isEmpty()) {
		blog(LOG_ERROR, "[StreamUP Record Chapter Manager] Chapter file path is not set, creating a new file.");
		createExportFiles();
		return;
	}

	QString fullChapterName = chapterName;

	// Ensure the chapter source is only appended once
	if (addChapterSourceEnabled && !chapterName.contains(chapterSource)) {
		fullChapterName += " (" + chapterSource + ")";
	}

	QString content = QString("%1 - %2\n").arg(timestamp, fullChapterName);
	if (!writeToFile(exportTextFilePath, content)) {
		blog(LOG_ERROR, "[StreamUP Record Chapter Manager] Failed to open text file: %s", QT_TO_UTF8(exportTextFilePath));
	}
}

void ChapterMarkerDock::setExportXMLFilePath(const QString &filePath)
{
	exportXMLFilePath = filePath;
}

void ChapterMarkerDock::writeChapterToXMLFile(const QString &chapterName, const QString &timestamp, const QString &chapterSource)
{
	if (!exportChaptersToFileEnabled || !exportChaptersToXMLEnabled) {
		return;
	}

	if (exportXMLFilePath.isEmpty()) {
		blog(LOG_ERROR, "[StreamUP Record Chapter Manager] XML file path is not set, creating a new file.");
		createExportFiles();
		return;
	}

	QFile file(exportXMLFilePath);
	if (!file.open(QIODevice::Append | QIODevice::Text)) {
		blog(LOG_ERROR, "[StreamUP Record Chapter Manager] Failed to open XML file: %s", QT_TO_UTF8(exportXMLFilePath));
		return;
	}

	QTextStream out(&file);
	out << "  <Chapter>\n";
	out << "    <Name>" << chapterName << "</Name>\n";
	out << "    <Timestamp>" << timestamp << "</Timestamp>\n";
	out << "    <Source>" << chapterSource << "</Source>\n";
	out << "  </Chapter>\n";
	file.close();
}

void ChapterMarkerDock::writeAnnotationToFiles(const QString &annotationText, const QString &timestamp,
					       const QString &annotationSource)
{
	if (!obs_frontend_recording_active()) {
		setAnnotationFeedbackLabel(obs_module_text("AnnotationErrorOutputNotActive"), "error");
		return;
	}

	if (!exportChaptersToFileEnabled) {
		setAnnotationFeedbackLabel(obs_module_text("NoExportMethod"), "error");
		return;
	}

	if (annotationText.isEmpty()) {
		setAnnotationFeedbackLabel(obs_module_text("AnnotationErrorTextIsEmpty"), "error");
		return;
	}

	// Check and create export files if paths are empty
	if (exportTextFilePath.isEmpty() || exportXMLFilePath.isEmpty()) {
		createExportFiles();
	}

	// Prepare the full annotation text including the source
	QString annotationName = QString::fromUtf8(obs_module_text("Annotation"));
	QString fullAnnotationText = "(" + annotationName + ") " + annotationText + " (" + annotationSource + ")";

	// Writing to text file if enabled
	if (exportChaptersToTextEnabled) {
		if (!writeToFile(exportTextFilePath, QString("%1 - %2\n").arg(timestamp, fullAnnotationText))) {
			blog(LOG_ERROR, "[StreamUP Record Chapter Manager] Failed to open text file: %s",
			     QT_TO_UTF8(exportTextFilePath));
			return;
		}
	}

	// Writing to XML file if enabled
	if (exportChaptersToXMLEnabled) {
		QString xmlContent = QString("<Annotation>\n"
					     "  <Timestamp>%1</Timestamp>\n"
					     "  <Text>%2</Text>\n"
					     "  <Source>%3</Source>\n"
					     "</Annotation>\n")
					     .arg(timestamp, annotationText, annotationSource);

		if (!writeToFile(exportXMLFilePath, xmlContent)) {
			blog(LOG_ERROR, "[StreamUP Record Chapter Manager] Failed to open XML file: %s",
			     QT_TO_UTF8(exportXMLFilePath));
			return;
		}
	}

	setAnnotationFeedbackLabel(obs_module_text("AnnotationSaved"), "good");
	annotationDock->annotationEdit->clear();

	// Emit WebSocket event for the new annotation
	obs_data_t *event_data = obs_data_create();
	obs_data_set_string(event_data, "annotationText", QT_TO_UTF8(annotationText));
	obs_data_set_string(event_data, "timestamp", QT_TO_UTF8(timestamp));
	obs_data_set_string(event_data, "annotationSource", QT_TO_UTF8(annotationSource));
	EmitWebSocketEvent("AnnotationSet", event_data);
	obs_data_release(event_data);
}

void ChapterMarkerDock::setAnnotationFeedbackLabel(const QString &text, const QString &themeID)
{
	annotationDock->feedbackLabel->setText(text);
	annotationDock->feedbackLabel->setProperty("themeID", themeID);
	style()->polish(annotationDock->feedbackLabel);
	annotationDock->feedbackTimer.start();
}

void ChapterMarkerDock::setChapterMarkerFeedbackLabel(const QString &text, const QString &themeID)
{
	feedbackLabel->setText(text);
	feedbackLabel->setProperty("themeID", themeID);
	style()->polish(feedbackLabel);
	feedbackTimer.start();
}

bool ChapterMarkerDock::writeToFile(const QString &filePath, const QString &content)
{
	QFile file(filePath);
	if (!file.open(QIODevice::Append | QIODevice::Text)) {
		return false;
	}

	QTextStream out(&file);
	out << content;
	file.close();
	return true;
}

//--------------------MISC EVENT HANDLERS--------------------
void ChapterMarkerDock::onSceneChanged()
{
	if (chapterOnSceneChangeEnabled && obs_frontend_recording_active()) {
		obs_source_t *current_scene = obs_frontend_get_current_scene();
		if (current_scene) {
			const char *scene_name = obs_source_get_name(current_scene);
			if (scene_name) {
				QString sceneName = QString::fromUtf8(scene_name);
				if (!ignoredScenes.contains(sceneName)) {
					addChapterMarker(sceneName, obs_module_text("ChangeScene"));
				}
				obs_source_release(current_scene);
			}
		}
	}
}

extern bool (*obs_frontend_recording_add_chapter_wrapper)(const char *name);

void ChapterMarkerDock::addChapterMarker(const QString &chapterName, const QString &chapterSource)
{
	QString fullChapterName = chapterName;
	QString sourceText = " (" + chapterSource + ")";

	if (addChapterSourceEnabled) {
		if (!fullChapterName.contains(sourceText)) {
			fullChapterName += sourceText;
		}
	}

	if (!isFirstRunInRecording && insertChapterMarkersInVideoEnabled) {
		if (obs_frontend_recording_add_chapter_wrapper) {
			bool success = obs_frontend_recording_add_chapter_wrapper(QT_TO_UTF8(fullChapterName));
			if (!success) {
				blog(LOG_INFO,
				     "[StreamUP Record Chapter Manager] You have selected to insert chapters into video file. You are not using a compatible file type.");

				if (!incompatibleFileTypeMessageShown) {
					QMessageBox msgBox;
					msgBox.setWindowTitle(obs_module_text("StreamUPChapterMarkerManagerError"));
					msgBox.setIcon(QMessageBox::Warning);
					msgBox.setText(obs_module_text("IncompatibleFileTypeError"));
					msgBox.setInformativeText(obs_module_text("IncompatibleFileType"));
					msgBox.setStandardButtons(QMessageBox::Ok);
					msgBox.setDefaultButton(QMessageBox::Ok);
					msgBox.exec();

					incompatibleFileTypeMessageShown = true;
				}
			}
		}
		auto ph = obs_get_proc_handler();
		calldata cd;
		calldata_init(&cd);
		calldata_set_string(&cd, "chapter_name", QT_TO_UTF8(fullChapterName));
		proc_handler_call(ph, "aitum_vertical_add_chapter", &cd);
		calldata_free(&cd);
	} // Log and handle the result of adding the chapter marker
	QString timestamp = getCurrentRecordingTime();

	// Always write to the chapter file if enabled
	if (exportChaptersToTextEnabled) {
		writeChapterToTextFile(chapterName, timestamp, chapterSource);
	}

	if (exportChaptersToXMLEnabled) {
		writeChapterToXMLFile(chapterName, timestamp, chapterSource);
	}

	blog(LOG_INFO, "[StreamUP Record Chapter Manager] Added chapter marker: %s", QT_TO_UTF8(fullChapterName));

	updateCurrentChapterLabel(fullChapterName);
	QString feedbackMessage = QString("%1 %2").arg(obs_module_text("NewChapter")).arg(fullChapterName);
	showFeedbackMessage(feedbackMessage, false);

	// Update the global current chapter name
	currentChapterName = fullChapterName;

	// Move the chapter to the top of the previous chapters list
	QList<QListWidgetItem *> items = previousChaptersList->findItems(fullChapterName, Qt::MatchExactly);
	if (!items.isEmpty()) {
		delete previousChaptersList->takeItem(previousChaptersList->row(items.first()));
	}
	previousChaptersList->insertItem(0, fullChapterName);

	chapters.insert(0, chapterName);
	timestamps.insert(0, timestamp);

	// Emit WebSocket event for the new chapter marker
	obs_data_t *event_data = obs_data_create();
	obs_data_set_string(event_data, "chapterName", QT_TO_UTF8(chapterName));
	obs_data_set_string(event_data, "chapterSource", QT_TO_UTF8(chapterSource));
	EmitWebSocketEvent("ChapterMarkerSet", event_data);
	obs_data_release(event_data);

	// After the first run, set the flag to false
	isFirstRunInRecording = false;
}

void ChapterMarkerDock::onAddChapterMarker(const QString &chapterName, const QString &chapterSource)
{
	addChapterMarker(chapterName, chapterSource);
}

void ChapterMarkerDock::onAddAnnotation(const QString &annotationText, const QString &annotationSource)
{
	writeAnnotationToFiles(annotationText, getCurrentRecordingTime(), annotationSource);
}

//--------------------UTILITY FUNCTIONS--------------------
void ChapterMarkerDock::applyThemeIDToButton(QPushButton *button, const QString &themeID)
{
	button->setProperty("themeID", themeID);
	button->setMinimumSize(32, 24);
	button->setMaximumSize(32, 24);
	button->setIconSize(QSize(20, 20));
	button->style()->unpolish(button);
	button->style()->polish(button);
}

QString ChapterMarkerDock::getChapterName() const
{
	return chapterNameInput->text();
}

QString ChapterMarkerDock::getCurrentRecordingTime() const
{
	obs_output_t *output = obs_frontend_get_recording_output();
	if (!output) {
		return QString("00:00:00");
	}

	uint64_t totalFrames = obs_output_get_total_frames(output);
	obs_video_info ovi;
	QString recordingTimeString = "00:00:00";

	if (obs_get_video_info(&ovi) != 0) {
		double frameRate = static_cast<double>(ovi.fps_num) / ovi.fps_den;
		uint64_t totalSeconds = static_cast<uint64_t>(totalFrames / frameRate);
		QTime recordingTime(0, 0, 0);
		recordingTime = recordingTime.addSecs(totalSeconds);
		recordingTimeString = recordingTime.toString("HH:mm:ss");
	}

	obs_output_release(output); // Ensure this is always called before returning

	return recordingTimeString;
}

//--------------------CONFIGS--------------------
void ChapterMarkerDock::LoadSettings(obs_data_t *settings)
{
	// Default chapter name
	defaultChapterName = QString::fromUtf8(obs_data_get_string(settings, "defaultChapterName"));
	useIncrementalChapterNames = obs_data_get_bool(settings, "useIncrementalChapterNames");

	// Set chapter on scene change
	chapterOnSceneChangeEnabled = obs_data_get_bool(settings, "chapterOnSceneChangeEnabled");

	// Previous chapters
	showPreviousChaptersEnabled = obs_data_get_bool(settings, "showPreviousChaptersEnabled");

	// Write to files
	exportChaptersToFileEnabled = obs_data_get_bool(settings, "exportChaptersToFileEnabled");
	exportChaptersToTextEnabled = obs_data_get_bool(settings, "exportChaptersToTextEnabled");
	exportChaptersToXMLEnabled = obs_data_get_bool(settings, "exportChaptersToXmlEnabled");

	// Write chapters to video
	insertChapterMarkersInVideoEnabled = obs_data_get_bool(settings, "insertChapterMarkersInVideoEnabled");

	// Add chapter source
	addChapterSourceEnabled = obs_data_get_bool(settings, "addChapterSourceEnabled");

	// Load ignored scenes
	ignoredScenes.clear();
	obs_data_array_t *ignoredScenesArray = obs_data_get_array(settings, "ignoredScenes");
	if (ignoredScenesArray) {
		size_t count = obs_data_array_count(ignoredScenesArray);
		for (size_t i = 0; i < count; ++i) {
			obs_data_t *sceneData = obs_data_array_item(ignoredScenesArray, i);
			const char *sceneName = obs_data_get_string(sceneData, "sceneName");
			if (sceneName) {
				ignoredScenes << QString::fromUtf8(sceneName);
			}
			obs_data_release(sceneData);
		}
		obs_data_array_release(ignoredScenesArray);
	}
}

void ChapterMarkerDock::SaveSettings()
{
	obs_data_t *settings = obs_data_create();

	// Default chapter name
	obs_data_set_string(settings, "defaultChapterName",
			    defaultChapterNameEdit->text().isEmpty() ? obs_module_text("Chapter")
								     : defaultChapterNameEdit->text().toStdString().c_str());
	obs_data_set_bool(settings, "useIncrementalChapterNames", useIncrementalChapterNames);

	// Set chapter on scene change
	obs_data_set_bool(settings, "chapterOnSceneChangeEnabled", chapterOnSceneChangeCheckbox->isChecked());

	// Previous chapters
	obs_data_set_bool(settings, "showPreviousChaptersEnabled", showPreviousChaptersCheckbox->isChecked());

	// Write to files
	obs_data_set_bool(settings, "exportChaptersToFileEnabled", exportChaptersToFileCheckbox->isChecked());
	obs_data_set_bool(settings, "exportChaptersToTextEnabled", exportChaptersToTextCheckbox->isChecked());
	obs_data_set_bool(settings, "exportChaptersToXmlEnabled", exportChaptersToXMLCheckbox->isChecked());

	// Write chapters to video
	obs_data_set_bool(settings, "insertChapterMarkersInVideoEnabled", insertChapterMarkersCheckbox->isChecked());

	// Add chapter source
	obs_data_set_bool(settings, "addChapterSourceEnabled", addChapterSourceCheckbox->isChecked());

	// Save ignored scenes
	obs_data_array_t *ignoredScenesArray = obs_data_array_create();
	for (const QString &sceneName : ignoredScenes) {
		obs_data_t *sceneData = obs_data_create();
		obs_data_set_string(sceneData, "sceneName", sceneName.toStdString().c_str());
		obs_data_array_push_back(ignoredScenesArray, sceneData);
		obs_data_release(sceneData);
	}
	obs_data_set_array(settings, "ignoredScenes", ignoredScenesArray);
	obs_data_array_release(ignoredScenesArray);

	SaveLoadSettingsCallback(settings, true);

	obs_data_release(settings);
}
