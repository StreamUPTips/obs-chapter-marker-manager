#include "chapter-marker-dock.hpp"
#include "annotation-dock.hpp"
#include "streamup-record-chapter-manager.hpp"
#include <obs-frontend-api.h>
#include <obs-module.h>
#include <obs-data.h>
#include <QHBoxLayout>
#include <QApplication>
#include <QStyle>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QComboBox>
#include <QListWidget>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFrame>
#include <QGroupBox>
#include <QMainWindow>

#define QT_TO_UTF8(str) str.toUtf8().constData()

//--------------------CONSTRUCTOR & DESTRUCTOR--------------------
ChapterMarkerDock::ChapterMarkerDock(QWidget *parent)
	: QFrame(parent),
	  chapterNameInput(new QLineEdit(this)),
	  saveButton(new QPushButton("Save Chapter Marker", this)),
	  settingsButton(new QPushButton(this)),
	  currentChapterTextLabel(new QLabel("Current Chapter:", this)),
	  currentChapterNameLabel(new QLabel("No Recording Active", this)),
	  feedbackLabel(new QLabel("", this)),
	  settingsDialog(nullptr),
	  chapterOnSceneChangeCheckbox(nullptr),
	  showPreviousChaptersCheckbox(nullptr),
	  exportChaptersToTextCheckbox(nullptr),
	  addChapterSourceCheckbox(nullptr),
	  ignoredScenesListWidget(nullptr),
	  chapterOnSceneChangeEnabled(false),
	  showPreviousChaptersEnabled(false),
	  exportChaptersToTextEnabled(false),
	  addChapterSourceEnabled(false),
	  previousChaptersList(new QListWidget(this)),
	  defaultChapterName("Chapter"),
	  chapterCount(1),
	  annotationButton(new QPushButton(this)),
	  annotationDock(nullptr),
	  ignoredScenesDialog(nullptr)
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
	// Enter Chapter Name text field and button
	connect(chapterNameInput, &QLineEdit::returnPressed, saveButton,
		&QPushButton::click);
	connect(saveButton, &QPushButton::clicked, this,
		&ChapterMarkerDock::onAddChapterMarkerButton);

	// Settings button
	connect(settingsButton, &QPushButton::clicked, this,
		&ChapterMarkerDock::onSettingsClicked);

	// Annotation button
	connect(annotationButton, &QPushButton::clicked, this,
		&ChapterMarkerDock::onAnnotationClicked);

	// Previous Chapter list
	connect(previousChaptersList, &QListWidget::itemClicked, this,
		&ChapterMarkerDock::onPreviousChapterSelected);
	connect(previousChaptersList, &QListWidget::itemDoubleClicked, this,
		&ChapterMarkerDock::onPreviousChapterDoubleClicked);

	// Feedback label timer
	feedbackTimer.setInterval(5000);
	feedbackTimer.setSingleShot(true);
	connect(&feedbackTimer, &QTimer::timeout,
		[this]() { feedbackLabel->setText(""); });
}

void ChapterMarkerDock::setupOBSCallbacks()
{
	obs_frontend_add_event_callback(
		[](obs_frontend_event event, void *ptr) {
			auto dock = static_cast<ChapterMarkerDock *>(ptr);
			if (event == OBS_FRONTEND_EVENT_SCENE_CHANGED ||
			    event == OBS_FRONTEND_EVENT_SCENE_LIST_CHANGED) {
				dock->onSceneChanged();
			} else if (event ==
				   OBS_FRONTEND_EVENT_RECORDING_STOPPED) {
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

void ChapterMarkerDock::setupMainDockCurrentChapterLayout(
	QVBoxLayout *mainLayout)
{
	QHBoxLayout *chapterLabelLayout = new QHBoxLayout();
	chapterLabelLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);

	currentChapterTextLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	currentChapterNameLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	currentChapterNameLabel->setWordWrap(true);
	currentChapterNameLabel->setSizePolicy(QSizePolicy::Expanding,
					       QSizePolicy::Preferred);
	currentChapterNameLabel->setStyleSheet("color: red;");

	chapterLabelLayout->addWidget(currentChapterTextLabel);
	chapterLabelLayout->addWidget(currentChapterNameLabel);

	mainLayout->addLayout(chapterLabelLayout);
}

void ChapterMarkerDock::setupMainDockChapterInput(QVBoxLayout *mainLayout)
{
	chapterNameInput->setPlaceholderText("Enter chapter name");
	mainLayout->addWidget(chapterNameInput);
}

void ChapterMarkerDock::setupMainDockSaveButtonLayout(QVBoxLayout *mainLayout)
{
	QHBoxLayout *saveButtonLayout = new QHBoxLayout();

	// Make Save Chapter Marker button fill the horizontal space
	saveButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

	// Configure the settings button
	applyThemeIDToButton(settingsButton, "configIconSmall");

	// Configure the annotation button using the applyThemeIDToButton function
	annotationButton->setIcon(QIcon(":images/annotation-icon.svg"));
	annotationButton->setMinimumSize(32, 24);
	annotationButton->setMaximumSize(32, 24);
	annotationButton->setIconSize(QSize(20, 20));

	// Add the Save Chapter Marker button and a stretch to push the settings button to the right
	saveButtonLayout->addWidget(saveButton);
	saveButtonLayout->addStretch();
	saveButtonLayout->addWidget(annotationButton);
	saveButtonLayout->addWidget(settingsButton);

	saveButtonLayout->setAlignment(saveButton, Qt::AlignLeft);
	saveButtonLayout->setAlignment(annotationButton, Qt::AlignRight);
	saveButtonLayout->setAlignment(settingsButton, Qt::AlignRight);

	mainLayout->addLayout(saveButtonLayout);
}

void ChapterMarkerDock::setupMainDockFeedbackLabel(QVBoxLayout *mainLayout)
{
	feedbackLabel->setStyleSheet("color: green;");
	feedbackLabel->setWordWrap(true);
	mainLayout->addWidget(feedbackLabel);
}

void ChapterMarkerDock::setupMainDockPreviousChaptersGroup(
	QVBoxLayout *mainLayout)
{
	previousChaptersGroup = new QGroupBox("Previous Chapters", this);
	previousChaptersGroup->setStyleSheet(
		"QGroupBox { font-weight: bold; border: 1px solid gray; padding: 10px; margin-top: 10px; } QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top center; padding: 0 3px; }");
	QVBoxLayout *previousChaptersLayout =
		new QVBoxLayout(previousChaptersGroup);
	previousChaptersLayout->setAlignment(
		Qt::AlignTop |
		Qt::AlignLeft); // Align previous chapters layout to top left
	previousChaptersLayout->addWidget(previousChaptersList);
	previousChaptersGroup->setLayout(previousChaptersLayout);

	mainLayout->addWidget(previousChaptersGroup);
}

void ChapterMarkerDock::refreshMainDockUI()
{
	previousChaptersGroup->setVisible(showPreviousChaptersEnabled);
	onAnnotationClicked(true);
}

//--------------------MAIN DOCK UI EVENT HANDLERS--------------------
void ChapterMarkerDock::onAddChapterMarkerButton()
{
	if (!obs_frontend_recording_active()) {
		blog(LOG_WARNING,
		     "[StreamUP Record Chapter Manager] Recording is not active. Chapter marker not added.");
		showFeedbackMessage(
			"Recording is not active. Chapter marker not added.",
			true);
		return;
	}

	QString chapterName = getChapterName();
	if (chapterName.isEmpty()) {
		// Use the default chapter name if the user did not provide one
		chapterName = defaultChapterName + " " +
			      QString::number(chapterCount);
		chapterCount++;
	}

	addChapterMarker(chapterName, "Manual");
	blog(LOG_INFO,
	     "[StreamUP Record Chapter Manager] Chapter marker added with name: %s",
	     chapterName.toStdString().c_str());
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
	showFeedbackMessage("Recording finished", false);

	QString timestamp = getCurrentRecordingTime();
	writeChapterToTextFile("End", timestamp, "Recording");

	clearPreviousChaptersGroup();
	blog(LOG_INFO, "[StreamUP Record Chapter Manager] chapterCount: %d",
	     chapterCount);

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
		saveButton->click();
	}
}

void ChapterMarkerDock::clearPreviousChaptersGroup()
{
	previousChaptersList->clear();
	chapters.clear();
	timestamps.clear();
}

void ChapterMarkerDock::onAnnotationClicked(bool refresh)
{
	if (refresh) {
		if (!exportChaptersToFileEnabled)
		{
			const char *dock_id = "AnnotationDock";
			obs_frontend_remove_dock(dock_id);
			annotationDock = nullptr;
			return;
		}
	}

	if (!exportChaptersToFileEnabled && !refresh) {
		showFeedbackMessage(
			"Please turn on 'Export chapters to .txt file' in settings to use Annotations.",
			true);
		return;
	}

	const auto main_window =
		static_cast<QMainWindow *>(obs_frontend_get_main_window());
	obs_frontend_push_ui_translation(obs_module_get_string);

	if (!annotationDock) {
		annotationDock = new AnnotationDock(this, main_window);

		const QString title = QString::fromUtf8(
			obs_module_text("StreamUP Annotation"));
		const auto name = "AnnotationDock";

#if LIBOBS_API_VER >= MAKE_SEMANTIC_VERSION(30, 0, 0)
		obs_frontend_add_dock_by_id(name, QT_TO_UTF8(title),
					    annotationDock);
#else
		auto dock = new QDockWidget(main_window);
		dock->setObjectName(name);
		dock->setWindowTitle(title);
		dock->setWidget(annotationDock);
		dock->setFeatures(QDockWidget::DockWidgetMovable |
				  QDockWidget::DockWidgetFloatable);
		dock->setFloating(true);
		obs_frontend_add_dock(dock);

		// Show the dock immediately after adding it
		dock->show();
		dock->raise();
#endif

		obs_frontend_pop_ui_translation();
	}

	// Show and raise the dock whether it was just created or already existed
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
	currentChapterNameLabel->setText(chapterName);
	currentChapterNameLabel->setStyleSheet("color: white;");
}

void ChapterMarkerDock::showFeedbackMessage(const QString &message,
					    bool isError)
{
	feedbackLabel->setWordWrap(true);
	feedbackLabel->setText(message);
	feedbackLabel->setStyleSheet(isError ? "color: red;" : "color: green;");
	feedbackTimer.start();
}

//--------------------SETTINGS UI--------------------
QDialog *ChapterMarkerDock::createSettingsUI()
{
	settingsDialog = new QDialog(this);
	settingsDialog->setWindowTitle("Settings");

	QVBoxLayout *mainLayout = new QVBoxLayout(settingsDialog);

	setupSettingsGeneralGroup(mainLayout);
	setupSettingsExportGroup(mainLayout);
	setupSettingsAutoChapterGroup(mainLayout);

	QDialogButtonBox *buttonBox = new QDialogButtonBox(
		QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
		settingsDialog);
	connect(buttonBox, &QDialogButtonBox::accepted, this,
		&ChapterMarkerDock::saveSettingsAndCloseDialog);
	connect(buttonBox, &QDialogButtonBox::rejected, settingsDialog,
		&QDialog::reject);
	mainLayout->addWidget(buttonBox);

	settingsDialog->setSizePolicy(QSizePolicy::Preferred,
				      QSizePolicy::Fixed);
	settingsDialog->adjustSize();

	return settingsDialog;
}

void ChapterMarkerDock::setupSettingsGeneralGroup(QVBoxLayout *mainLayout)
{
	QGroupBox *generalSettingsGroup =
		new QGroupBox("General Settings", settingsDialog);
	QVBoxLayout *generalSettingsLayout =
		new QVBoxLayout(generalSettingsGroup);

	QLabel *defaultChapterNameLabel =
		new QLabel("Default Chapter Name:", generalSettingsGroup);
	defaultChapterNameEdit = new QLineEdit(generalSettingsGroup);
	defaultChapterNameEdit->setPlaceholderText(
		"Enter default chapter name");
	defaultChapterNameEdit->setText(defaultChapterName);
	defaultChapterNameEdit->setMinimumWidth(200);

	// Create a horizontal layout to place the label and text box side by side
	QHBoxLayout *chapterNameLayout = new QHBoxLayout;
	chapterNameLayout->addWidget(defaultChapterNameLabel);
	chapterNameLayout->addWidget(defaultChapterNameEdit);

	// Add the horizontal layout to the general settings layout
	generalSettingsLayout->addLayout(chapterNameLayout);

	showPreviousChaptersCheckbox =
		new QCheckBox("Show chapter history", generalSettingsGroup);
	generalSettingsLayout->addWidget(showPreviousChaptersCheckbox);
	showPreviousChaptersCheckbox->setChecked(showPreviousChaptersEnabled);

	addChapterSourceCheckbox = new QCheckBox("Add chapter trigger source",
						 generalSettingsGroup);
	generalSettingsLayout->addWidget(addChapterSourceCheckbox);
	addChapterSourceCheckbox->setChecked(addChapterSourceEnabled);

	generalSettingsGroup->setLayout(generalSettingsLayout);
	generalSettingsGroup->adjustSize();

	// Set the size policy to Preferred for width and Fixed for height
	generalSettingsGroup->setSizePolicy(QSizePolicy::Preferred,
					    QSizePolicy::Fixed);

	mainLayout->addWidget(generalSettingsGroup);
}

void ChapterMarkerDock::setupSettingsExportGroup(QVBoxLayout *mainLayout)
{
	exportSettingsGroup = new QGroupBox("Export");
	exportSettingsLayout = new QVBoxLayout(exportSettingsGroup);

	// Insert chapter markers into video file checkbox
	insertChapterMarkersCheckbox = new QCheckBox(
		"Insert Chapter Markers into Video File", exportSettingsGroup);
	exportSettingsLayout->addWidget(insertChapterMarkersCheckbox);
	insertChapterMarkersCheckbox->setChecked(
		insertChapterMarkersInVideoEnabled);

	// Create check boxes
	exportChaptersToFileCheckbox = new QCheckBox(
		"Export Chapter Markers to File", exportSettingsGroup);
	exportChaptersToTextCheckbox =
		new QCheckBox("Export to .txt", exportSettingsGroup);
	exportChaptersToXMLCheckbox =
		new QCheckBox("Export to .xml", exportSettingsGroup);

	// Set check boxes visually
	exportChaptersToFileCheckbox->setChecked(exportChaptersToFileEnabled);
	exportChaptersToTextCheckbox->setChecked(exportChaptersToTextEnabled);
	exportChaptersToXMLCheckbox->setChecked(exportChaptersToXMLEnabled);
	exportChaptersToTextCheckbox->setVisible(exportChaptersToFileEnabled);
	exportChaptersToXMLCheckbox->setVisible(exportChaptersToFileEnabled);

	connect(exportChaptersToFileCheckbox, &QCheckBox::toggled, this,
		&ChapterMarkerDock::onExportChaptersToFileToggled);

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
	exportSettingsGroup->setSizePolicy(QSizePolicy::Preferred,
					   QSizePolicy::Fixed);

	mainLayout->addWidget(exportSettingsGroup);
}

void ChapterMarkerDock::setupSettingsAutoChapterGroup(QVBoxLayout *mainLayout)
{
	sceneChangeSettingsGroup =
		new QGroupBox("Automatic Chapter Settings", settingsDialog);
	QVBoxLayout *sceneChangeSettingsLayout =
		new QVBoxLayout(sceneChangeSettingsGroup);

	// Add chapter on scene change checkbox
	chapterOnSceneChangeCheckbox = new QCheckBox(
		"Set Chapter on Scene Change", sceneChangeSettingsGroup);
	sceneChangeSettingsLayout->addWidget(chapterOnSceneChangeCheckbox);
	chapterOnSceneChangeCheckbox->setChecked(chapterOnSceneChangeEnabled);
	connect(chapterOnSceneChangeCheckbox, &QCheckBox::toggled, this,
		&ChapterMarkerDock::onChapterOnSceneChangeToggled);

	// Set ignore scene button
	setIgnoredScenesButton =
		new QPushButton("Set Ignored Scenes", sceneChangeSettingsGroup);
	setIgnoredScenesButton->setVisible(chapterOnSceneChangeEnabled);
	connect(setIgnoredScenesButton, &QPushButton::clicked, this,
		&ChapterMarkerDock::onSetIgnoredScenesClicked);
	sceneChangeSettingsLayout->addWidget(setIgnoredScenesButton);

	sceneChangeSettingsGroup->setLayout(sceneChangeSettingsLayout);
	sceneChangeSettingsGroup->setSizePolicy(QSizePolicy::Preferred,
						QSizePolicy::Fixed);

	mainLayout->addWidget(sceneChangeSettingsGroup);
}

void ChapterMarkerDock::saveSettingsAndCloseDialog()
{
	exportChaptersToFileEnabled = exportChaptersToFileCheckbox->isChecked(); 
	SaveSettings();
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

//--------------------IGNORED SCENES UI--------------------
QDialog *ChapterMarkerDock::createIgnoredScenesUI()
{
	QDialog *dialog = new QDialog(this);
	dialog->setWindowTitle("Ignored Scenes");

	QVBoxLayout *mainLayout = new QVBoxLayout(dialog);

	ignoredScenesListWidget = new QListWidget(dialog);
	ignoredScenesListWidget->setSelectionMode(
		QAbstractItemView::MultiSelection);

	// Disable horizontal scrolling
	ignoredScenesListWidget->setHorizontalScrollBarPolicy(
		Qt::ScrollBarAlwaysOff);

	// Populate the list widget with OBS scene names
	populateIgnoredScenesListWidget();

	// Adjust the width of the list widget to fit the contents
	int longestNameWidth = ignoredScenesListWidget->sizeHintForColumn(0);
	ignoredScenesListWidget->setFixedWidth(longestNameWidth +
					       30); // Add some padding

	// Calculate the height for 10 entries
	int rowHeight = ignoredScenesListWidget->sizeHintForRow(0);
	int listHeight = rowHeight * 10;
	ignoredScenesListWidget->setFixedHeight(listHeight);

	mainLayout->addWidget(ignoredScenesListWidget);

	// Create OK and Cancel buttons
	QDialogButtonBox *buttonBox = new QDialogButtonBox(
		QDialogButtonBox::Ok | QDialogButtonBox::Cancel, dialog);
	connect(buttonBox, &QDialogButtonBox::accepted, dialog,
		&QDialog::accept);
	connect(buttonBox, &QDialogButtonBox::rejected, dialog,
		&QDialog::reject);

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
			ignoredScenesListWidget->addItem(
				QString::fromUtf8(*name));
		}
		bfree(scene_names);
	}
}

//--------------------FILE MANAGEMENT--------------------
void ChapterMarkerDock::createExportFiles()
{
	obs_output_t *output = obs_frontend_get_recording_output();
	if (!output) {
		blog(LOG_ERROR,
		     "[StreamUP Record Chapter Manager] Could not get the recording output.");
		return;
	}

	obs_data_t *settings = obs_output_get_settings(output);
	if (!settings) {
		blog(LOG_ERROR,
		     "[StreamUP Record Chapter Manager] Could not get the recording output settings.");
		obs_output_release(output);
		return;
	}

	const char *recording_path = obs_data_get_string(settings, "path");
	if (!recording_path || strlen(recording_path) == 0) {
		blog(LOG_ERROR,
		     "[StreamUP Record Chapter Manager] Could not get the recording output path.");
		obs_data_release(settings);
		obs_output_release(output);
		return;
	}

	QString outputPath = QString::fromUtf8(recording_path);
	obs_data_release(settings);
	obs_output_release(output);

	blog(LOG_INFO, "[StreamUP Record Chapter Manager] Recording path: %s",
	     QT_TO_UTF8(outputPath));

	QFileInfo fileInfo(outputPath);
	QString baseName = fileInfo.completeBaseName();
	QString directoryPath = fileInfo.absolutePath();

	if (exportChaptersToTextEnabled) {
		QString chapterFilePath =
			directoryPath + "/" + baseName + "_chapters.txt";
		QFile file(chapterFilePath);
		if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
			QTextStream out(&file);
			out << "Chapter Markers for " << baseName << "\n";
			file.close();
			blog(LOG_INFO,
			     "[StreamUP Record Chapter Manager] Created chapter file: %s",
			     QT_TO_UTF8(chapterFilePath));
			setExportTextFilePath(chapterFilePath);
		} else {
			blog(LOG_ERROR,
			     "[StreamUP Record Chapter Manager] Failed to create chapter file: %s",
			     QT_TO_UTF8(chapterFilePath));
		}
	}

	if (exportChaptersToXMLEnabled) {
		QString xmlFilePath =
			directoryPath + "/" + baseName + "_chapters.xml";
		QFile xmlFile(xmlFilePath);
		if (xmlFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
			QTextStream out(&xmlFile);
			out << "<ChapterMarkers>\n";
			out << "  <Title>" << baseName << "</Title>\n";
			out << "</ChapterMarkers>\n";
			xmlFile.close();
			blog(LOG_INFO,
			     "[StreamUP Record Chapter Manager] Created XML chapter file: %s",
			     QT_TO_UTF8(xmlFilePath));
			setExportXMLFilePath(xmlFilePath);
		} else {
			blog(LOG_ERROR,
			     "[StreamUP Record Chapter Manager] Failed to create XML chapter file: %s",
			     QT_TO_UTF8(xmlFilePath));
		}
	}
}

void ChapterMarkerDock::setExportTextFilePath(const QString &filePath)
{
	exportTextFilePath = filePath;
}

void ChapterMarkerDock::writeChapterToTextFile(const QString &chapterName,
					       const QString &timestamp,
					       const QString &chapterSource)
{
	if (!exportChaptersToTextEnabled || !exportChaptersToFileEnabled) {
		return;
	}

	if (exportTextFilePath.isEmpty()) {
		blog(LOG_ERROR,
		     "[StreamUP Record Chapter Manager] Chapter file path is not set, creating a new file.");
		createExportFiles();
		return;
	}

	QFile file(exportTextFilePath);
	if (file.open(QIODevice::Append | QIODevice::Text)) {
		QTextStream out(&file);
		QString fullChapterName = chapterName;

		// Ensure the chapter source is only appended once
		if (addChapterSourceEnabled &&
		    !chapterName.contains(chapterSource)) {
			fullChapterName += " (" + chapterSource + ")";
		}

		out << timestamp << " - " << fullChapterName << "\n";
		file.close();
	} else {
		blog(LOG_ERROR,
		     "[StreamUP Record Chapter Manager] Failed to open chapter file: %s",
		     QT_TO_UTF8(exportTextFilePath));
	}
}

void ChapterMarkerDock::setExportXMLFilePath(const QString &filePath)
{
	exportXMLFilePath = filePath;
}

void ChapterMarkerDock::writeChapterToXMLFile(const QString &chapterName,
					      const QString &timestamp,
					      const QString &chapterSource)
{
	if (!exportChaptersToXMLEnabled || !exportChaptersToFileEnabled) {
		return;
	}

	if (exportXMLFilePath.isEmpty()) {
		blog(LOG_ERROR,
		     "[StreamUP Record Chapter Manager] Chapter file path is not set, creating a new file.");
		createExportFiles();
		return;
	}

	QFile file(exportXMLFilePath);
	if (file.open(QIODevice::Append | QIODevice::Text)) {
		QTextStream out(&file);
		QString fullChapterName = chapterName;

		if (addChapterSourceEnabled &&
		    !chapterName.contains(chapterSource)) {
			fullChapterName += " (" + chapterSource + ")";
		}

		out << "<Chapter>\n";
		out << "  <Timestamp>" << timestamp << "</Timestamp>\n";
		out << "  <Name>" << fullChapterName << "</Name>\n";
		out << "</Chapter>\n";

		file.close();
	} else {
		blog(LOG_ERROR,
		     "[StreamUP Record Chapter Manager] Failed to open XML file: %s",
		     QT_TO_UTF8(exportXMLFilePath));
	}
}

void ChapterMarkerDock::writeAnnotationToFiles(const QString &annotationText,
					       const QString &timestamp,
					       const QString &chapterSource)
{
	if (!exportChaptersToFileEnabled) {
		return;
	}

	// Writing to text file if enabled
	if (exportChaptersToTextEnabled) {
		if (exportTextFilePath.isEmpty()) {
			blog(LOG_ERROR,
			     "[StreamUP Record Chapter Manager] Text file path is not set, creating a new file.");
			createExportFiles();
		}

		QFile textFile(exportTextFilePath);
		if (textFile.open(QIODevice::Append | QIODevice::Text)) {
			QTextStream out(&textFile);
			QString fullAnnotationText =
				"(Annotation) " + annotationText;
			out << timestamp << " - " << fullAnnotationText << "\n";
			textFile.close();
		} else {
			blog(LOG_ERROR,
			     "[StreamUP Record Chapter Manager] Failed to open text file: %s",
			     QT_TO_UTF8(exportTextFilePath));
		}
	}

	// Writing to XML file if enabled
	if (exportChaptersToXMLEnabled) {
		if (exportXMLFilePath.isEmpty()) {
			blog(LOG_ERROR,
			     "[StreamUP Record Chapter Manager] XML file path is not set, creating a new file.");
			createExportFiles();
		}

		QFile xmlFile(exportXMLFilePath);
		if (xmlFile.open(QIODevice::Append | QIODevice::Text)) {
			QTextStream out(&xmlFile);
			out << "<Annotation>\n";
			out << "  <Timestamp>" << timestamp << "</Timestamp>\n";
			out << "  <Text>" << annotationText << "</Text>\n";
			out << "</Annotation>\n";
			xmlFile.close();
		} else {
			blog(LOG_ERROR,
			     "[StreamUP Record Chapter Manager] Failed to open XML file: %s",
			     QT_TO_UTF8(exportXMLFilePath));
		}
	}
}

//--------------------MISC EVENT HANDLERS--------------------
void ChapterMarkerDock::onSceneChanged()
{
	if (chapterOnSceneChangeEnabled && obs_frontend_recording_active()) {
		obs_source_t *current_scene = obs_frontend_get_current_scene();
		if (current_scene) {
			const char *scene_name =
				obs_source_get_name(current_scene);
			if (scene_name) {
				QString sceneName =
					QString::fromUtf8(scene_name);
				if (!ignoredScenes.contains(sceneName)) {
					addChapterMarker(sceneName,
							 "Scene Change");
				}
				obs_source_release(current_scene);
			}
		}
	}
}

void ChapterMarkerDock::addChapterMarker(const QString &chapterName,
					 const QString &chapterSource)
{
	QString fullChapterName = chapterName;
	QString sourceText = " (" + chapterSource + ")";

	if (addChapterSourceEnabled) {
		if (!fullChapterName.contains(sourceText)) {
			fullChapterName += sourceText;
		}
	}

	// Add chapter marker to the recording
	bool success =
		obs_frontend_recording_add_chapter(QT_TO_UTF8(fullChapterName));

	// Log and handle the result of adding the chapter marker
	if (success) {
		QString timestamp = getCurrentRecordingTime();

		// Always write to the chapter file if enabled
		if (exportChaptersToTextEnabled) {
			writeChapterToTextFile(chapterName, timestamp,
					       chapterSource);
		}

		if (exportChaptersToXMLEnabled) {
			writeChapterToXMLFile(chapterName, timestamp,
					      chapterSource);
		}

		blog(LOG_INFO,
		     "[StreamUP Record Chapter Manager] Added chapter marker: %s",
		     QT_TO_UTF8(fullChapterName));
		updateCurrentChapterLabel(fullChapterName);
		showFeedbackMessage(
			QString("Chapter marker added: %1").arg(fullChapterName),
			false);

		// Move the chapter to the top of the previous chapters list
		QList<QListWidgetItem *> items =
			previousChaptersList->findItems(fullChapterName,
							Qt::MatchExactly);
		if (!items.isEmpty()) {
			delete previousChaptersList->takeItem(
				previousChaptersList->row(items.first()));
		}
		previousChaptersList->insertItem(0, fullChapterName);

		chapters.insert(0, chapterName);
		timestamps.insert(0, timestamp);
	} else {
		blog(LOG_ERROR,
		     "[StreamUP Record Chapter Manager] Failed to add chapter marker. Ensure the output supports chapter markers.");
		showFeedbackMessage(
			"Failed to add chapter marker. Ensure the output supports chapter markers.",
			true);
	}
}

//--------------------UTILITY FUNCTIONS--------------------
void ChapterMarkerDock::applyThemeIDToButton(QPushButton *button,
					     const QString &themeID)
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
		double frameRate =
			static_cast<double>(ovi.fps_num) / ovi.fps_den;
		uint64_t totalSeconds =
			static_cast<uint64_t>(totalFrames / frameRate);
		QTime recordingTime(0, 0, 0);
		recordingTime = recordingTime.addSecs(totalSeconds);
		recordingTimeString = recordingTime.toString("HH:mm:ss");
	}

	obs_output_release(
		output); // Ensure this is always called before returning

	return recordingTimeString;
}

//--------------------CONFIGS--------------------
void ChapterMarkerDock::LoadSettings(obs_data_t *settings)
{
	// Default chapter name
	defaultChapterName = QString::fromUtf8(
		obs_data_get_string(settings, "default_chapter_name"));

	// Set chapter on scene change
	chapterOnSceneChangeEnabled =
		obs_data_get_bool(settings, "chapter_on_scene_change_enabled");

	// Previous chapters
	showPreviousChaptersEnabled =
		obs_data_get_bool(settings, "show_previous_chapters_enabled");

	// Write to files
	exportChaptersToFileEnabled =
		obs_data_get_bool(settings, "export_chapters_to_file_enabled");
	exportChaptersToTextEnabled =
		obs_data_get_bool(settings, "export_chapters_to_text_enabled");
	exportChaptersToXMLEnabled =
		obs_data_get_bool(settings, "export_chapters_to_xml_enabled");

	// Write chapters to video
	insertChapterMarkersInVideoEnabled = obs_data_get_bool(
		settings, "insert_chapter_markers_in_video_enabled");

	// Add chapter source
	addChapterSourceEnabled =
		obs_data_get_bool(settings, "add_chapter_source_enabled");

	// Load ignored scenes
	ignoredScenes.clear();
	obs_data_array_t *ignoredScenesArray =
		obs_data_get_array(settings, "ignored_scenes");
	if (ignoredScenesArray) {
		size_t count = obs_data_array_count(ignoredScenesArray);
		for (size_t i = 0; i < count; ++i) {
			obs_data_t *sceneData =
				obs_data_array_item(ignoredScenesArray, i);
			const char *sceneName =
				obs_data_get_string(sceneData, "scene_name");
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

	//Default chapter name
	obs_data_set_string(
		settings, "default_chapter_name",
		defaultChapterNameEdit->text().isEmpty()
			? "Chapter"
			: defaultChapterNameEdit->text().toStdString().c_str());

	// Set chapter on scene change
	obs_data_set_bool(settings, "chapter_on_scene_change_enabled",
			  chapterOnSceneChangeCheckbox->isChecked());

	// Previous chapters
	obs_data_set_bool(settings, "show_previous_chapters_enabled",
			  showPreviousChaptersCheckbox->isChecked());

	// Write to files
	obs_data_set_bool(settings, "export_chapters_to_file_enabled",
			  exportChaptersToFileCheckbox->isChecked());
	obs_data_set_bool(settings, "export_chapters_to_text_enabled",
			  exportChaptersToTextCheckbox->isChecked());
	obs_data_set_bool(settings, "export_chapters_to_xml_enabled",
			  exportChaptersToXMLCheckbox->isChecked());

	// Write chapters to video
	obs_data_set_bool(settings, "insert_chapter_markers_in_video_enabled",
			  insertChapterMarkersCheckbox->isChecked());

	// Add chapter source
	obs_data_set_bool(settings, "add_chapter_source_enabled",
			  addChapterSourceCheckbox->isChecked());

	SaveLoadSettingsCallback(settings, true);

	obs_data_release(settings);
}
