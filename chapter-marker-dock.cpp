#include "chapter-marker-dock.hpp"
#include <obs-frontend-api.h>
#include <obs-module.h>
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


#define QT_TO_UTF8(str) str.toUtf8().constData()

ChapterMarkerDock::ChapterMarkerDock(QWidget *parent)
	: QFrame(parent),
	  chapterNameEdit(new QLineEdit(this)),
	  saveButton(new QPushButton("Save Chapter Marker", this)),
	  settingsButton(new QPushButton(this)), // Create the settings button
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
	  showChapterHistoryEnabled(true), // Default to true
	  exportChaptersEnabled(false),    // Default to false
	  addChapterSourceEnabled(false),  // Default to false
	  chapterHistoryList(new QListWidget(this)),
	  defaultChapterName("Chapter"), // Default to "Chapter"
	  chapterCount(1)                // Initialize chapter count to 1
{
	// Set the frame style
	this->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);

	QVBoxLayout *mainLayout = new QVBoxLayout(this);
	mainLayout->setAlignment(
		Qt::AlignTop | Qt::AlignLeft); // Align main layout to top left

	QHBoxLayout *chapterLabelLayout = new QHBoxLayout();
	chapterLabelLayout->setAlignment(
		Qt::AlignTop |
		Qt::AlignLeft); // Align chapter label layout to top left

	currentChapterTextLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	currentChapterNameLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	currentChapterNameLabel->setWordWrap(true);
	currentChapterNameLabel->setStyleSheet("color: red;");

	chapterLabelLayout->addWidget(currentChapterTextLabel);
	chapterLabelLayout->addWidget(currentChapterNameLabel);

	chapterNameEdit->setPlaceholderText("Enter chapter name");
	connect(chapterNameEdit, &QLineEdit::returnPressed, saveButton,
		&QPushButton::click);

	connect(saveButton, &QPushButton::clicked, this,
		&ChapterMarkerDock::onSaveClicked);

	feedbackLabel->setStyleSheet("color: green;");

	QHBoxLayout *saveButtonLayout = new QHBoxLayout();

	// Make Save Chapter Marker button fill the horizontal space
	saveButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

	// Configure the settings button
	settingsButton->setMinimumSize(32, 24);
	settingsButton->setMaximumSize(32, 24);
	settingsButton->setProperty("themeID", "configIconSmall");
	settingsButton->setIconSize(QSize(20, 20));
	settingsButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

	// Add the Save Chapter Marker button and a stretch to push the settings button to the right
	saveButtonLayout->addWidget(saveButton);
	saveButtonLayout->addStretch();
	saveButtonLayout->addWidget(settingsButton);

	saveButtonLayout->setAlignment(saveButton, Qt::AlignLeft);
	saveButtonLayout->setAlignment(settingsButton, Qt::AlignRight);

	connect(settingsButton, &QPushButton::clicked, this,
		&ChapterMarkerDock::onSettingsClicked);

	mainLayout->addLayout(chapterLabelLayout);
	mainLayout->addWidget(chapterNameEdit);
	mainLayout->addLayout(saveButtonLayout);
	mainLayout->addWidget(feedbackLabel);

	// GroupBox for Previous Chapters
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

	setLayout(mainLayout);

	connect(chapterHistoryList, &QListWidget::itemClicked, this,
		&ChapterMarkerDock::onHistoryItemSelected);
	connect(chapterHistoryList, &QListWidget::itemDoubleClicked, this,
		&ChapterMarkerDock::onHistoryItemDoubleClicked);

	feedbackTimer.setInterval(5000);
	feedbackTimer.setSingleShot(true);
	connect(&feedbackTimer, &QTimer::timeout,
		[this]() { feedbackLabel->setText(""); });

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

	// Initialize the visibility of the chapter history based on the default setting
	previousChaptersGroup->setVisible(showChapterHistoryEnabled);
}

ChapterMarkerDock::~ChapterMarkerDock()
{
	if (settingsDialog) {
		delete settingsDialog;
	}
}

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
	feedbackLabel->setText(message);
	feedbackLabel->setStyleSheet(isError ? "color: red;" : "color: green;");
	feedbackTimer.start();
}

void ChapterMarkerDock::onSaveClicked()
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
	if (!chapterName.isEmpty()) {
		addChapterMarker(chapterName, "Manual");
		chapterNameEdit->clear();
	} else {
		blog(LOG_WARNING,
		     "[StreamUP Record Chapter Manager] Chapter name is empty. Marker not added.");
		showFeedbackMessage("Chapter name is empty. Marker not added.",
				    true);
	}
}

void ChapterMarkerDock::onSettingsClicked()
{
	if (!settingsDialog) {
		settingsDialog = new QDialog(this);
		settingsDialog->setWindowTitle("Settings");
		settingsDialog->setFixedSize(
			400, 350); // Adjust the size for new settings

		QVBoxLayout *mainLayout = new QVBoxLayout(settingsDialog);

		// GroupBox for General Settings
		QGroupBox *generalSettingsGroup =
			new QGroupBox("General Settings", settingsDialog);
		QVBoxLayout *generalSettingsLayout =
			new QVBoxLayout(generalSettingsGroup);

		// Add new setting for default chapter marker name
		QLabel *defaultChapterNameLabel = new QLabel(
			"Default Chapter Name:", generalSettingsGroup);
		defaultChapterNameEdit = new QLineEdit(generalSettingsGroup);
		defaultChapterNameEdit->setPlaceholderText(
			"Enter default chapter name");
		defaultChapterNameEdit->setText(defaultChapterName);
		generalSettingsLayout->addWidget(defaultChapterNameLabel);
		generalSettingsLayout->addWidget(defaultChapterNameEdit);

		showChapterHistoryCheckbox = new QCheckBox(
			"Show chapter history", generalSettingsGroup);
		generalSettingsLayout->addWidget(showChapterHistoryCheckbox);

		exportChaptersCheckbox = new QCheckBox(
			"Export chapters to .txt file", generalSettingsGroup);
		generalSettingsLayout->addWidget(exportChaptersCheckbox);

		addChapterSourceCheckbox = new QCheckBox(
			"Add chapter trigger source", generalSettingsGroup);
		generalSettingsLayout->addWidget(addChapterSourceCheckbox);

		generalSettingsGroup->setLayout(generalSettingsLayout);

		// GroupBox for Chapter on Scene Change Settings
		QGroupBox *sceneChangeSettingsGroup =
			new QGroupBox("Scene Change Settings", settingsDialog);
		QVBoxLayout *sceneChangeSettingsLayout =
			new QVBoxLayout(sceneChangeSettingsGroup);

		chapterOnSceneChangeCheckbox =
			new QCheckBox("Set chapter on scene change",
				      sceneChangeSettingsGroup);
		sceneChangeSettingsLayout->addWidget(
			chapterOnSceneChangeCheckbox);

		// GroupBox for Ignored Scenes
		ignoredScenesGroup = new QGroupBox(
			"Ignored Scenes",
			sceneChangeSettingsGroup); // Initialize the member variable
		ignoredScenesGroup->setStyleSheet(
			"QGroupBox { font-weight: bold; border: 1px solid gray; padding: 10px; margin-top: 10px; } QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top center; padding: 0 3px; }");
		QVBoxLayout *ignoredScenesLayout =
			new QVBoxLayout(ignoredScenesGroup);

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

		// Add groups to main layout
		mainLayout->addWidget(generalSettingsGroup);
		mainLayout->addWidget(sceneChangeSettingsGroup);

		QDialogButtonBox *buttonBox = new QDialogButtonBox(
			QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
			settingsDialog);
		connect(buttonBox, &QDialogButtonBox::accepted, settingsDialog,
			&QDialog::accept);
		connect(buttonBox, &QDialogButtonBox::rejected, settingsDialog,
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

		connect(settingsDialog, &QDialog::accepted, [this]() {
			chapterOnSceneChangeEnabled =
				chapterOnSceneChangeCheckbox->isChecked();
			showChapterHistoryEnabled =
				showChapterHistoryCheckbox->isChecked();
			exportChaptersEnabled =
				exportChaptersCheckbox->isChecked();
			addChapterSourceEnabled =
				addChapterSourceCheckbox->isChecked();
			defaultChapterName =
				defaultChapterNameEdit->text().isEmpty()
					? "Chapter"
					: defaultChapterNameEdit
						  ->text(); // Save the default chapter name

			ignoredScenes.clear();
			for (int i = 0; i < ignoredScenesListWidget->count();
			     ++i) {
				QListWidgetItem *item =
					ignoredScenesListWidget->item(i);
				if (item->isSelected()) {
					ignoredScenes << item->text();
				}
			}

			// Update the visibility of the chapter history list based on the setting
			previousChaptersGroup->setVisible(
				showChapterHistoryEnabled);
		});
	}

	if (chapterOnSceneChangeCheckbox && showChapterHistoryCheckbox &&
	    exportChaptersCheckbox && addChapterSourceCheckbox &&
	    ignoredScenesListWidget) {
		chapterOnSceneChangeCheckbox->setChecked(
			chapterOnSceneChangeEnabled);
		showChapterHistoryCheckbox->setChecked(
			showChapterHistoryEnabled);
		exportChaptersCheckbox->setChecked(exportChaptersEnabled);
		addChapterSourceCheckbox->setChecked(addChapterSourceEnabled);
		defaultChapterNameEdit->setText(
			defaultChapterName); // Initialize the text

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

		settingsDialog->exec();
	}
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

void ChapterMarkerDock::clearChapterHistory()
{
	chapterHistoryList->clear();
	chapters.clear();
	timestamps.clear();
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
		if (addChapterSourceEnabled) {
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

void ChapterMarkerDock::setAddChapterSourceEnabled(bool enabled)
{
	addChapterSourceEnabled = enabled;
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
		if (!fullChapterName.endsWith(sourceText)) {
			fullChapterName += sourceText;
		}
	}

	// Check for duplicates in the chapter history list
	for (int i = 0; i < chapterHistoryList->count(); ++i) {
		if (chapterHistoryList->item(i)->text() == fullChapterName) {
			return; // Do not add if duplicate is found
		}
	}

	if (obs_frontend_recording_add_chapter(QT_TO_UTF8(fullChapterName))) {
		QString timestamp = getCurrentRecordingTime();
		writeChapterToFile(chapterName, timestamp, chapterSource);

		blog(LOG_INFO,
		     "[StreamUP Record Chapter Manager] Added chapter marker: %s",
		     QT_TO_UTF8(fullChapterName));
		updateCurrentChapterLabel(fullChapterName);
		showFeedbackMessage(
			QString("Chapter marker added: %1").arg(fullChapterName),
			false);

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

void ChapterMarkerDock::onHotkeyTriggered()
{
	if (!obs_frontend_recording_active()) {
		setNoRecordingActive();
		showFeedbackMessage(
			"Recording is not active. Chapter marker not added.",
			true);
		return;
	}

	QString chapterName =
		defaultChapterName + " " + QString::number(chapterCount);
	addChapterMarker(chapterName, "Hotkey");
	blog(LOG_INFO, "[StreamUP Record Chapter Manager] chapterCount: %d",
	     chapterCount);

	chapterCount++; // Increment the chapter count
}

void ChapterMarkerDock::populateIgnoredScenesComboBox()
{
	if (!ignoredScenesComboBox) {
		return;
	}

	ignoredScenesComboBox->clear();

	char **scene_names = obs_frontend_get_scene_names();
	if (scene_names) {
		for (char **name = scene_names; *name; name++) {
			ignoredScenesComboBox->addItem(
				QString::fromUtf8(*name));
		}
		bfree(scene_names); // Free the allocated memory
	}
}

void ChapterMarkerDock::setIgnoredScenes(const QStringList &scenes)
{
	ignoredScenes = scenes;
	if (ignoredScenesListWidget) {
		for (int i = 0; i < ignoredScenesListWidget->count(); ++i) {
			QListWidgetItem *item =
				ignoredScenesListWidget->item(i);
			item->setSelected(ignoredScenes.contains(item->text()));
		}
	}
}

QStringList ChapterMarkerDock::getIgnoredScenes() const
{
	return ignoredScenes;
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
