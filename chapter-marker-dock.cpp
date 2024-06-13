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
	  chapterNameEdit(new QLineEdit(this)),
	  saveButton(new QPushButton("Save Chapter Marker", this)),
	  settingsButton(new QPushButton(this)),
	  currentChapterTextLabel(new QLabel("Current Chapter:", this)),
	  currentChapterNameLabel(new QLabel("No Recording Active", this)),
	  feedbackLabel(new QLabel("", this)),
	  settingsDialog(nullptr),
	  chapterOnSceneChangeCheckbox(nullptr),
	  showChapterHistoryCheckbox(nullptr),
	  exportChaptersCheckbox(nullptr),
	  addChapterSourceCheckbox(nullptr),
	  ignoredScenesListWidget(nullptr),
	  chapterOnSceneChangeEnabled(false),
	  showChapterHistoryEnabled(false),
	  exportChaptersEnabled(false),
	  addChapterSourceEnabled(false),
	  chapterHistoryList(new QListWidget(this)),
	  defaultChapterName("Chapter"),
	  chapterCount(1),
	  annotationButton(new QPushButton(this)),
	  annotationDock(nullptr)
{
	// UI Setup
	setupUI();
	// Signal Connections
	setupConnections();
	// OBS Event Callback
	setupOBSCallbacks();
	// initialise UI States
	initialiseUIStates();
}

ChapterMarkerDock::~ChapterMarkerDock()
{
	if (settingsDialog) {
		delete settingsDialog;
	}
}

//--------------------UI SETUP FUNCTIONS--------------------
void ChapterMarkerDock::setupUI()
{
	// Set the frame style
	this->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);

	QVBoxLayout *mainLayout = new QVBoxLayout(this);
	mainLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	mainLayout->setSizeConstraint(QLayout::SetMinimumSize);

	setupCurrentChapterLayout(mainLayout);
	setupChapterNameEdit(mainLayout);
	setupSaveButtonLayout(mainLayout);
	setupFeedbackLabel(mainLayout);
	setupPreviousChaptersGroup(mainLayout);

	setLayout(mainLayout);
}

void ChapterMarkerDock::setupCurrentChapterLayout(QVBoxLayout *mainLayout)
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

void ChapterMarkerDock::setupChapterNameEdit(QVBoxLayout *mainLayout)
{
	chapterNameEdit->setPlaceholderText("Enter chapter name");
	mainLayout->addWidget(chapterNameEdit);
}

void ChapterMarkerDock::setupSaveButtonLayout(QVBoxLayout *mainLayout)
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

void ChapterMarkerDock::setupFeedbackLabel(QVBoxLayout *mainLayout)
{
	feedbackLabel->setStyleSheet("color: green;");
	feedbackLabel->setWordWrap(true);
	mainLayout->addWidget(feedbackLabel);
}

void ChapterMarkerDock::setupPreviousChaptersGroup(QVBoxLayout *mainLayout)
{
	previousChaptersGroup = new QGroupBox("Previous Chapters", this);
	previousChaptersGroup->setStyleSheet(
		"QGroupBox { font-weight: bold; border: 1px solid gray; padding: 10px; margin-top: 10px; } QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top center; padding: 0 3px; }");
	QVBoxLayout *previousChaptersLayout =
		new QVBoxLayout(previousChaptersGroup);
	previousChaptersLayout->setAlignment(
		Qt::AlignTop |
		Qt::AlignLeft); // Align previous chapters layout to top left
	previousChaptersLayout->addWidget(chapterHistoryList);
	previousChaptersGroup->setLayout(previousChaptersLayout);

	mainLayout->addWidget(previousChaptersGroup);
}

//--------------------SIGNAL CONNECTIONS--------------------
void ChapterMarkerDock::setupConnections()
{
	connect(chapterNameEdit, &QLineEdit::returnPressed, saveButton,
		&QPushButton::click);
	connect(saveButton, &QPushButton::clicked, this,
		&ChapterMarkerDock::onAddChapterMarkerButton);
	connect(settingsButton, &QPushButton::clicked, this,
		&ChapterMarkerDock::onSettingsClicked);
	connect(annotationButton, &QPushButton::clicked, this,
		&ChapterMarkerDock::
			onAnnotationClicked); // Connect annotation button

	connect(chapterHistoryList, &QListWidget::itemClicked, this,
		&ChapterMarkerDock::onHistoryItemSelected);
	connect(chapterHistoryList, &QListWidget::itemDoubleClicked, this,
		&ChapterMarkerDock::onHistoryItemDoubleClicked);

	feedbackTimer.setInterval(5000);
	feedbackTimer.setSingleShot(true);
	connect(&feedbackTimer, &QTimer::timeout,
		[this]() { feedbackLabel->setText(""); });
}

//--------------------INITIALISE UI STATES--------------------
void ChapterMarkerDock::initialiseUIStates()
{
	// initialise the visibility of the chapter history based on the default setting
	previousChaptersGroup->setVisible(showChapterHistoryEnabled);
}

//--------------------EVENT HANDLERS--------------------
void ChapterMarkerDock::onAddChapterMarkerButton()
{
	if (!obs_frontend_recording_active()) {
		blog(LOG_WARNING,
		     "[StreamUP Record Chapter Manager] Recording is not active. Chapter marker not added.");
		setNoRecordingActive();
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
		chapterCount++; // Increment the chapter count
	}

	addChapterMarker(chapterName, "Manual");
	blog(LOG_INFO,
	     "[StreamUP Record Chapter Manager] Chapter marker added with name: %s",
	     chapterName.toStdString().c_str());
	chapterNameEdit->clear();
}

void ChapterMarkerDock::onSettingsClicked()
{
	if (!settingsDialog) {
		settingsDialog = createSettingsDialog();
	}
	initialiseSettingsDialog();
	settingsDialog->exec();
}

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
	} else if (!obs_frontend_recording_active()) {
		setNoRecordingActive();
	}
}

void ChapterMarkerDock::onRecordingStopped()
{
	setNoRecordingActive();
	showFeedbackMessage("Recording finished", false);

	QString timestamp = getCurrentRecordingTime();
	writeChapterToFile("End", timestamp, "Recording");

	clearChapterHistory();
	blog(LOG_INFO, "[StreamUP Record Chapter Manager] chapterCount: %d",
	     chapterCount);

	chapterCount = 1; // Reset chapter count to 1
}

void ChapterMarkerDock::onHistoryItemSelected()
{
	QListWidgetItem *currentItem = chapterHistoryList->currentItem();
	if (currentItem) {
		chapterNameEdit->setText(currentItem->text());
	}
}

void ChapterMarkerDock::onHistoryItemDoubleClicked(QListWidgetItem *item)
{
	if (item) {
		chapterNameEdit->setText(item->text());
		saveButton->click();
	}
}

//--------------------UTILITY FUNCTIONS--------------------
QString ChapterMarkerDock::getChapterName() const
{
	return chapterNameEdit->text();
}

void ChapterMarkerDock::updateCurrentChapterLabel(const QString &chapterName)
{
	currentChapterNameLabel->setText(chapterName);
	currentChapterNameLabel->setStyleSheet("color: white;");
}

void ChapterMarkerDock::setNoRecordingActive()
{
	currentChapterNameLabel->setText("No Recording Active");
	currentChapterNameLabel->setStyleSheet("color: red;");
}

void ChapterMarkerDock::showFeedbackMessage(const QString &message,
					    bool isError)
{
	feedbackLabel->setWordWrap(true);
	feedbackLabel->setText(message);
	feedbackLabel->setStyleSheet(isError ? "color: red;" : "color: green;");
	feedbackTimer.start();
}

void ChapterMarkerDock::clearChapterHistory()
{
	chapterHistoryList->clear();
	chapters.clear();
	timestamps.clear();
}

void ChapterMarkerDock::writeChapterToFile(const QString &chapterName,
					   const QString &timestamp,
					   const QString &chapterSource)
{
	if (!exportChaptersEnabled) {
		return;
	}

	if (chapterFilePath.isEmpty()) {
		blog(LOG_ERROR,
		     "[StreamUP Record Chapter Manager] Chapter file path is not set.");
		return;
	}

	QFile file(chapterFilePath);
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
		     QT_TO_UTF8(chapterFilePath));
	}
}

void ChapterMarkerDock::setChapterFilePath(const QString &filePath)
{
	chapterFilePath = filePath;
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

bool ChapterMarkerDock::isAddChapterSourceEnabled() const
{
	return addChapterSourceEnabled;
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
		if (exportChaptersEnabled) {
			writeChapterToFile(chapterName, timestamp,
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
		QList<QListWidgetItem *> items = chapterHistoryList->findItems(
			fullChapterName, Qt::MatchExactly);
		if (!items.isEmpty()) {
			delete chapterHistoryList->takeItem(
				chapterHistoryList->row(items.first()));
		}
		chapterHistoryList->insertItem(0, fullChapterName);

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

void ChapterMarkerDock::populateIgnoredScenesListWidget()
{
	if (!ignoredScenesListWidget) {
		return;
	}

	ignoredScenesListWidget->clear();

	char **scene_names = obs_frontend_get_scene_names();
	if (scene_names) {
		for (char **name = scene_names; *name; name++) {
			ignoredScenesListWidget->addItem(
				QString::fromUtf8(*name));
		}
		bfree(scene_names); // Free the allocated memory
	}
}

void ChapterMarkerDock::updatePreviousChaptersVisibility(bool visible)
{
	previousChaptersGroup->setVisible(visible);
}

//--------------------CREATE SETTINGS DIALOG--------------------
QDialog *ChapterMarkerDock::createSettingsDialog()
{
	QDialog *dialog = new QDialog(this);
	dialog->setWindowTitle("Settings");
	dialog->setFixedSize(400, 350); // Adjust the size for new settings

	QVBoxLayout *mainLayout = new QVBoxLayout(dialog);

	setupGeneralSettingsGroup(mainLayout);
	setupSceneChangeSettingsGroup(mainLayout);

	QDialogButtonBox *buttonBox = new QDialogButtonBox(
		QDialogButtonBox::Ok | QDialogButtonBox::Cancel, dialog);

	connect(buttonBox, &QDialogButtonBox::accepted, this,
		&ChapterMarkerDock::saveSettingsAndCloseDialog);

	connect(buttonBox, &QDialogButtonBox::rejected, dialog,
		&QDialog::reject);
	mainLayout->addWidget(buttonBox);

	// Connect the checkbox state change to toggle the visibility of the ignored scenes group and adjust the dialog size
	connect(chapterOnSceneChangeCheckbox, &QCheckBox::toggled,
		[this](bool checked) {
			ignoredScenesGroup->setVisible(checked);
			if (checked) {
				settingsDialog->setMinimumSize(
					400,
					550); // Adjust the minimum size when ignored scenes are visible
				settingsDialog->setMaximumSize(
					QWIDGETSIZE_MAX,
					QWIDGETSIZE_MAX); // Allow resizing
			} else {
				settingsDialog->setFixedSize(
					400,
					350); // Reset to the fixed size when ignored scenes are hidden
			}
		});

	connect(dialog, &QDialog::accepted, [this]() {
		chapterOnSceneChangeEnabled =
			chapterOnSceneChangeCheckbox->isChecked();
		showChapterHistoryEnabled =
			showChapterHistoryCheckbox->isChecked();
		exportChaptersEnabled = exportChaptersCheckbox->isChecked();
		addChapterSourceEnabled = addChapterSourceCheckbox->isChecked();
		defaultChapterName =
			defaultChapterNameEdit->text().isEmpty()
				? "Chapter"
				: defaultChapterNameEdit
					  ->text(); // Save the default chapter name

		ignoredScenes.clear();
		for (int i = 0; i < ignoredScenesListWidget->count(); ++i) {
			QListWidgetItem *item =
				ignoredScenesListWidget->item(i);
			if (item->isSelected()) {
				ignoredScenes << item->text();
			}
		}

		// Update the visibility of the chapter history list based on the setting
		previousChaptersGroup->setVisible(showChapterHistoryEnabled);
	});

	return dialog;
}

//--------------------SETUP SETTINGS DIALOG GROUPS--------------------
void ChapterMarkerDock::setupGeneralSettingsGroup(QVBoxLayout *mainLayout)
{
	QGroupBox *generalSettingsGroup =
		new QGroupBox("General Settings", settingsDialog);
	QVBoxLayout *generalSettingsLayout =
		new QVBoxLayout(generalSettingsGroup);

	// Add new setting for default chapter marker name
	QLabel *defaultChapterNameLabel =
		new QLabel("Default Chapter Name:", generalSettingsGroup);
	defaultChapterNameEdit = new QLineEdit(generalSettingsGroup);
	defaultChapterNameEdit->setPlaceholderText(
		"Enter default chapter name");
	defaultChapterNameEdit->setText(defaultChapterName);
	generalSettingsLayout->addWidget(defaultChapterNameLabel);
	generalSettingsLayout->addWidget(defaultChapterNameEdit);

	showChapterHistoryCheckbox =
		new QCheckBox("Show chapter history", generalSettingsGroup);
	generalSettingsLayout->addWidget(showChapterHistoryCheckbox);

	exportChaptersCheckbox = new QCheckBox("Export chapters to .txt file",
					       generalSettingsGroup);
	generalSettingsLayout->addWidget(exportChaptersCheckbox);

	addChapterSourceCheckbox = new QCheckBox("Add chapter trigger source",
						 generalSettingsGroup);
	generalSettingsLayout->addWidget(addChapterSourceCheckbox);

	generalSettingsGroup->setLayout(generalSettingsLayout);

	mainLayout->addWidget(generalSettingsGroup);
}

void ChapterMarkerDock::setupSceneChangeSettingsGroup(QVBoxLayout *mainLayout)
{
	QGroupBox *sceneChangeSettingsGroup =
		new QGroupBox("Scene Change Settings", settingsDialog);
	QVBoxLayout *sceneChangeSettingsLayout =
		new QVBoxLayout(sceneChangeSettingsGroup);

	chapterOnSceneChangeCheckbox = new QCheckBox(
		"Set chapter on scene change", sceneChangeSettingsGroup);
	sceneChangeSettingsLayout->addWidget(chapterOnSceneChangeCheckbox);

	// GroupBox for Ignored Scenes
	ignoredScenesGroup =
		new QGroupBox("Ignored Scenes", sceneChangeSettingsGroup);
	ignoredScenesGroup->setStyleSheet(
		"QGroupBox { font-weight: bold; border: 1px solid gray; padding: 10px; margin-top: 10px; } QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top center; padding: 0 3px; }");
	QVBoxLayout *ignoredScenesLayout = new QVBoxLayout(ignoredScenesGroup);

	ignoredScenesListWidget = new QListWidget(ignoredScenesGroup);
	ignoredScenesListWidget->setSelectionMode(
		QAbstractItemView::MultiSelection);
	ignoredScenesListWidget->setMinimumHeight(
		100); // Set a minimum height for the ignored scenes list
	ignoredScenesLayout->addWidget(ignoredScenesListWidget);

	ignoredScenesGroup->setLayout(ignoredScenesLayout);
	ignoredScenesGroup->setVisible(false); // Initially hidden
	sceneChangeSettingsLayout->addWidget(ignoredScenesGroup);

	sceneChangeSettingsGroup->setLayout(sceneChangeSettingsLayout);

	mainLayout->addWidget(sceneChangeSettingsGroup);
}

void ChapterMarkerDock::saveSettingsAndCloseDialog()
{
	// Create a new obs_data_t object
	obs_data_t *saveData = obs_data_create();

	// Extract settings from UI elements
	obs_data_set_bool(saveData, "chapter_on_scene_change_enabled",
			  chapterOnSceneChangeCheckbox->isChecked());
	obs_data_set_bool(saveData, "show_chapter_history_enabled",
			  showChapterHistoryCheckbox->isChecked());
	obs_data_set_bool(saveData, "export_chapters_enabled",
			  exportChaptersCheckbox->isChecked());
	obs_data_set_bool(saveData, "add_chapter_source_enabled",
			  addChapterSourceCheckbox->isChecked());
	obs_data_set_string(
		saveData, "default_chapter_name",
		defaultChapterNameEdit->text().isEmpty()
			? "Chapter"
			: defaultChapterNameEdit->text().toStdString().c_str());

	// Create an array to store ignored scenes
	obs_data_array_t *ignoredScenesArray = obs_data_array_create();

	// Add ignored scenes to the array
	for (int i = 0; i < ignoredScenesListWidget->count(); ++i) {
		QListWidgetItem *item = ignoredScenesListWidget->item(i);
		if (item->isSelected()) {
			obs_data_t *sceneData = obs_data_create();
			obs_data_set_string(sceneData, "scene_name",
					    item->text().toStdString().c_str());
			obs_data_array_push_back(ignoredScenesArray, sceneData);
			obs_data_release(
				sceneData); // Release after adding to array
		}
	}

	// Add the array of ignored scenes to the save data
	obs_data_set_array(saveData, "ignored_scenes", ignoredScenesArray);

	// Save settings
	SaveLoadSettingsCallback(saveData, true);

	// Release resources
	obs_data_array_release(ignoredScenesArray);
	obs_data_release(saveData);

	// Check if export chapters is disabled and annotation dock exists
	if (!exportChaptersCheckbox->isChecked() && annotationDock) {
		const char *dock_id = "AnnotationDock";
		obs_frontend_remove_dock(dock_id);
		annotationDock = nullptr;
	}

	// Close the dialog
	settingsDialog->accept();
}

//--------------------INITIALISE SETTINGS DIALOG STATES--------------------
void ChapterMarkerDock::initialiseSettingsDialog()
{
	if (chapterOnSceneChangeCheckbox && showChapterHistoryCheckbox &&
	    exportChaptersCheckbox && addChapterSourceCheckbox &&
	    ignoredScenesListWidget) {
		chapterOnSceneChangeCheckbox->setChecked(
			chapterOnSceneChangeEnabled);
		showChapterHistoryCheckbox->setChecked(
			showChapterHistoryEnabled);
		exportChaptersCheckbox->setChecked(exportChaptersEnabled);
		addChapterSourceCheckbox->setChecked(addChapterSourceEnabled);
		defaultChapterNameEdit->setText(defaultChapterName);

		populateIgnoredScenesListWidget();
		for (int i = 0; i < ignoredScenesListWidget->count(); ++i) {
			QListWidgetItem *item =
				ignoredScenesListWidget->item(i);
			if (ignoredScenes.contains(item->text())) {
				item->setSelected(true);
			}
		}

		// Set initial visibility of ignored scenes group based on the checkbox state
		ignoredScenesGroup->setVisible(chapterOnSceneChangeEnabled);
		if (chapterOnSceneChangeEnabled) {
			settingsDialog->setMinimumSize(
				400,
				550); // Adjust the minimum size when ignored scenes are visible
			settingsDialog->setMaximumSize(
				QWIDGETSIZE_MAX,
				QWIDGETSIZE_MAX); // Allow resizing
		} else {
			settingsDialog->setFixedSize(
				400,
				350); // Reset to the fixed size when ignored scenes are hidden
		}
	}
}

//--------------------OBS EVENT CALLBACK--------------------
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

//--------------------SET SETTINGS--------------------
void ChapterMarkerDock::setAddChapterSourceEnabled(bool enabled)
{
	addChapterSourceEnabled = enabled;
}

void ChapterMarkerDock::applySettings(obs_data_t *settings)
{
	chapterOnSceneChangeEnabled =
		obs_data_get_bool(settings, "chapter_on_scene_change_enabled");
	showChapterHistoryEnabled =
		obs_data_get_bool(settings, "show_chapter_history_enabled");
	exportChaptersEnabled =
		obs_data_get_bool(settings, "export_chapters_enabled");
	addChapterSourceEnabled =
		obs_data_get_bool(settings, "add_chapter_source_enabled");
	defaultChapterName = QString::fromUtf8(
		obs_data_get_string(settings, "default_chapter_name"));

	// Populate the ignored scenes list from the settings
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

	// Update UI states based on the loaded settings
	initialiseUIStates();
}

void ChapterMarkerDock::onAnnotationClicked(bool startup)
{
	if (!exportChaptersEnabled && !startup) {
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
	if (!startup) {
		annotationDock->parentWidget()->show();
		annotationDock->parentWidget()->raise();
	}
}

void ChapterMarkerDock::setAnnotationDock(AnnotationDock *dock)
{
	annotationDock = dock;
}

void ChapterMarkerDock::writeAnnotationToFile(const QString &annotationText,
					      const QString &timestamp,
					      const QString &chapterSource)
{
	if (!exportChaptersEnabled) {
		return;
	}

	if (chapterFilePath.isEmpty()) {
		blog(LOG_ERROR,
		     "[StreamUP Record Chapter Manager] Chapter file path is not set.");
		return;
	}

	QFile file(chapterFilePath);
	if (file.open(QIODevice::Append | QIODevice::Text)) {
		QTextStream out(&file);
		QString fullAnnotationText = "(Annotation) " + annotationText;
		out << timestamp << " - " << fullAnnotationText << "\n";
		file.close();
	} else {
		blog(LOG_ERROR,
		     "[StreamUP Record Chapter Manager] Failed to open chapter file: %s",
		     QT_TO_UTF8(chapterFilePath));
	}
}

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
