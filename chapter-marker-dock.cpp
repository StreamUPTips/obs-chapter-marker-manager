#include "chapter-marker-dock.hpp"
#include <obs-frontend-api.h>
#include <obs-module.h>
#include <QHBoxLayout>
#include <QApplication>
#include <QStyle>
#include <QFile>
#include <QTextStream>
#include <QDir>

#define QT_TO_UTF8(str) str.toUtf8().constData()

ChapterMarkerDock::ChapterMarkerDock(QWidget *parent)
	: QWidget(parent),
	  chapterNameEdit(new QLineEdit(this)),
	  saveButton(new QPushButton("Save Chapter Marker", this)),
	  settingsButton(new QPushButton("Settings", this)),
	  currentChapterTextLabel(new QLabel("Current Chapter:", this)),
	  currentChapterNameLabel(new QLabel("No Recording Active", this)),
	  feedbackLabel(new QLabel("", this)),
	  settingsDialog(nullptr),
	  chapterOnSceneChangeCheckbox(nullptr),
	  showChapterHistoryCheckbox(nullptr),
	  exportChaptersCheckbox(nullptr),
	  addChapterSourceCheckbox(nullptr),
	  chapterOnSceneChangeEnabled(false),
	  showChapterHistoryEnabled(true), // Default to true
	  exportChaptersEnabled(false),    // Default to false
	  addChapterSourceEnabled(false),  // Default to false
	  historyLabel(new QLabel("Previous Chapters:", this)),
	  chapterHistoryList(new QListWidget(this))
{
	QVBoxLayout *mainLayout = new QVBoxLayout(this);

	QHBoxLayout *chapterLabelLayout = new QHBoxLayout();
	chapterLabelLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);

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

	mainLayout->addLayout(chapterLabelLayout);
	mainLayout->addWidget(chapterNameEdit);
	mainLayout->addWidget(saveButton);
	mainLayout->addWidget(settingsButton);
	mainLayout->addWidget(feedbackLabel);

	// Add the history label and list to the layout
	mainLayout->addWidget(historyLabel);
	mainLayout->addWidget(chapterHistoryList);

	setLayout(mainLayout);

	connect(settingsButton, &QPushButton::clicked, this,
		&ChapterMarkerDock::onSettingsClicked);
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
	historyLabel->setVisible(showChapterHistoryEnabled);
	chapterHistoryList->setVisible(showChapterHistoryEnabled);
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

		QFormLayout *formLayout = new QFormLayout(settingsDialog);
		chapterOnSceneChangeCheckbox = new QCheckBox(
			"Set chapter on scene change", settingsDialog);
		formLayout->addRow(chapterOnSceneChangeCheckbox);

		showChapterHistoryCheckbox =
			new QCheckBox("Show chapter history", settingsDialog);
		formLayout->addRow(showChapterHistoryCheckbox);

		exportChaptersCheckbox = new QCheckBox(
			"Export chapters automatically", settingsDialog);
		formLayout->addRow(exportChaptersCheckbox);

		addChapterSourceCheckbox = new QCheckBox(
			"Add chapter trigger source", settingsDialog);
		formLayout->addRow(addChapterSourceCheckbox);

		QDialogButtonBox *buttonBox = new QDialogButtonBox(
			QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
			settingsDialog);
		connect(buttonBox, &QDialogButtonBox::accepted, settingsDialog,
			&QDialog::accept);
		connect(buttonBox, &QDialogButtonBox::rejected, settingsDialog,
			&QDialog::reject);

		formLayout->addWidget(buttonBox);

		connect(settingsDialog, &QDialog::accepted, [this]() {
			chapterOnSceneChangeEnabled =
				chapterOnSceneChangeCheckbox->isChecked();
			showChapterHistoryEnabled =
				showChapterHistoryCheckbox->isChecked();
			exportChaptersEnabled =
				exportChaptersCheckbox->isChecked();
			addChapterSourceEnabled =
				addChapterSourceCheckbox->isChecked();

			// Update the visibility of the chapter history list based on the setting
			historyLabel->setVisible(showChapterHistoryEnabled);
			chapterHistoryList->setVisible(
				showChapterHistoryEnabled);
		});
	}

	if (chapterOnSceneChangeCheckbox && showChapterHistoryCheckbox &&
	    exportChaptersCheckbox && addChapterSourceCheckbox) {
		chapterOnSceneChangeCheckbox->setChecked(
			chapterOnSceneChangeEnabled);
		showChapterHistoryCheckbox->setChecked(
			showChapterHistoryEnabled);
		exportChaptersCheckbox->setChecked(exportChaptersEnabled);
		addChapterSourceCheckbox->setChecked(addChapterSourceEnabled);
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
				addChapterMarker(sceneName, "Scene Change");
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
        double frameRate = static_cast<double>(ovi.fps_num) / ovi.fps_den;
        uint64_t totalSeconds = static_cast<uint64_t>(totalFrames / frameRate);
        QTime recordingTime(0, 0, 0);
        recordingTime = recordingTime.addSecs(totalSeconds);
        recordingTimeString = recordingTime.toString("HH:mm:ss");
    }

    obs_output_release(output); // Ensure this is always called before returning

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
	if (addChapterSourceEnabled) {
		fullChapterName += " (" + chapterSource + ")";
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
