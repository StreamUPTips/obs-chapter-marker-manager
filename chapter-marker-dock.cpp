#include "chapter-marker-dock.hpp"
#include <obs-frontend-api.h>
#include <obs-module.h>
#include <QHBoxLayout>
#include <QApplication>
#include <QStyle>

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
	  chapterOnSceneChangeEnabled(false),
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
	mainLayout->addWidget(chapterHistoryList);

	setLayout(mainLayout);

	connect(settingsButton, &QPushButton::clicked, this,
		&ChapterMarkerDock::onSettingsClicked);
	connect(chapterHistoryList, &QListWidget::itemClicked, this,
		&ChapterMarkerDock::onHistoryItemSelected);

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
			}
		},
		this);
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
		if (obs_frontend_recording_add_chapter(
			    QT_TO_UTF8(chapterName))) {
			blog(LOG_INFO,
			     "[StreamUP Record Chapter Manager] Added chapter marker: %s",
			     QT_TO_UTF8(chapterName));
			updateCurrentChapterLabel(chapterName);
			showFeedbackMessage(QString("Chapter marker added: %1")
						    .arg(chapterName),
					    false);

			// Insert the chapter at the top of the history list
			chapterHistoryList->insertItem(0, chapterName);

			chapterNameEdit->clear();
		} else {
			blog(LOG_ERROR,
			     "[StreamUP Record Chapter Manager] Failed to add chapter marker. Ensure the output supports chapter markers.");
			showFeedbackMessage(
				"Failed to add chapter marker. Ensure the output supports chapter markers.",
				true);
		}
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
		});
	}

	if (chapterOnSceneChangeCheckbox) {
		chapterOnSceneChangeCheckbox->setChecked(
			chapterOnSceneChangeEnabled);
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
				if (obs_frontend_recording_add_chapter(
					    scene_name)) {
					blog(LOG_INFO,
					     "[StreamUP Record Chapter Manager] Added chapter marker for scene: %s",
					     scene_name);
					updateCurrentChapterLabel(
						QString::fromUtf8(scene_name));
					showFeedbackMessage(
						QString("Chapter marker added for scene: %1")
							.arg(scene_name),
						false);

					// Insert the chapter at the top of the history list
					chapterHistoryList->insertItem(
						0,
						QString::fromUtf8(scene_name));
				} else {
					blog(LOG_ERROR,
					     "[StreamUP Record Chapter Manager] Failed to add chapter marker for scene: %s. Ensure the output supports chapter markers.",
					     scene_name);
					showFeedbackMessage(
						QString("Failed to add chapter marker for scene: %1. Ensure the output supports chapter markers.")
							.arg(scene_name),
						true);
				}
			}
			obs_source_release(current_scene);
		}
	} else if (!obs_frontend_recording_active()) {
		setNoRecordingActive();
	}
}

void ChapterMarkerDock::onHistoryItemSelected()
{
	QListWidgetItem *item = chapterHistoryList->currentItem();
	if (item) {
		chapterNameEdit->setText(item->text());
	}
}
