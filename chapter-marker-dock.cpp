#include "chapter-marker-dock.hpp"
#include <obs-frontend-api.h>
#include <obs-module.h>
#include <QHBoxLayout>

#define QT_TO_UTF8(str) str.toUtf8().constData()

ChapterMarkerDock::ChapterMarkerDock(QWidget *parent) : QWidget(parent)
{
	QVBoxLayout *mainLayout = new QVBoxLayout(this);

	QHBoxLayout *chapterLabelLayout = new QHBoxLayout();
	chapterLabelLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);

	currentChapterTextLabel = new QLabel("Current Chapter:", this);
	currentChapterTextLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);

	currentChapterNameLabel = new QLabel("No Recording Active", this);
	currentChapterNameLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	currentChapterNameLabel->setWordWrap(true);
	currentChapterNameLabel->setStyleSheet("color: red;");

	chapterLabelLayout->addWidget(currentChapterTextLabel);
	chapterLabelLayout->addWidget(currentChapterNameLabel);

	chapterNameEdit = new QLineEdit(this);
	chapterNameEdit->setPlaceholderText("Enter chapter name");

	saveButton = new QPushButton("Save Chapter Marker", this);
	connect(saveButton, &QPushButton::clicked, this,
		&ChapterMarkerDock::onSaveClicked);

	mainLayout->addLayout(chapterLabelLayout);
	mainLayout->addWidget(chapterNameEdit);
	mainLayout->addWidget(saveButton);
	setLayout(mainLayout);
}

ChapterMarkerDock::~ChapterMarkerDock()
{
	// Cleanup if necessary
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

void ChapterMarkerDock::onSaveClicked()
{
	if (!obs_frontend_recording_active()) {
		blog(LOG_WARNING,
		     "[StreamUP Record Chapter Manager] Recording is not active. Chapter marker not added.");
		setNoRecordingActive();
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
		} else {
			blog(LOG_ERROR,
			     "[StreamUP Record Chapter Manager] Failed to add chapter marker. Ensure the output supports chapter markers.");
		}
	} else {
		blog(LOG_WARNING,
		     "[StreamUP Record Chapter Manager] Chapter name is empty. Marker not added.");
	}
}
