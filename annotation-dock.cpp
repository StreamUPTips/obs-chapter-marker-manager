#include "annotation-dock.hpp"
#include "chapter-marker-dock.hpp"
#include <QFile>
#include <QTextStream>
#include <obs-module.h>

#define QT_TO_UTF8(str) str.toUtf8().constData()

AnnotationDock::AnnotationDock(ChapterMarkerDock *chapterDock, QWidget *parent)
	: QFrame(parent),
	  annotationEdit(new QTextEdit(this)),
	  saveChapterMarkerButton(
		  new QPushButton(obs_module_text("SaveAnnotationText"), this)),
	  feedbackLabel(new QLabel("", this)),
	  chapterDock(chapterDock)
{
	setupUI();
	setupConnections();
	updateInputState(
		chapterDock
			->exportChaptersToFileEnabled); // Initialize the input state
}

AnnotationDock::~AnnotationDock() {}

void AnnotationDock::setupUI()
{
	QVBoxLayout *mainLayout = new QVBoxLayout(this);
	mainLayout->addWidget(annotationEdit);
	saveChapterMarkerButton->setToolTip(
		obs_module_text("SaveAnnotationButtonToolTip"));
	mainLayout->addWidget(saveChapterMarkerButton);
	mainLayout->addWidget(feedbackLabel);

	feedbackLabel->setProperty("themeID", "good");
	setLayout(mainLayout);
}

void AnnotationDock::setupConnections()
{
	connect(saveChapterMarkerButton, &QPushButton::clicked, this,
		&AnnotationDock::onSaveAnnotationButton);

	feedbackTimer.setInterval(5000);
	feedbackTimer.setSingleShot(true);
	connect(&feedbackTimer, &QTimer::timeout,
		[this]() { feedbackLabel->setText(""); });
}

void AnnotationDock::onSaveAnnotationButton()
{
	if (!obs_frontend_recording_active()) {
		feedbackLabel->setText(
			obs_module_text("AnnotationErrorOutputNotActive"));
		feedbackLabel->setProperty("themeID", "error");
		feedbackTimer.start();
		return;
	}

	QString annotationText = annotationEdit->toPlainText();
	if (annotationText.isEmpty()) {
		feedbackLabel->setText(
			obs_module_text("AnnotationErrorTextIsEmpty"));
		feedbackLabel->setProperty("themeID", "error");
		feedbackTimer.start();
		return;
	}

	QString timestamp = chapterDock->getCurrentRecordingTime();
	chapterDock->writeAnnotationToFiles(annotationText, timestamp,
					    obs_module_text("SourceManual"));

	feedbackLabel->setText(obs_module_text("AnnotationSaved"));
	feedbackLabel->setProperty("themeID", "good");

	feedbackTimer.start();
	annotationEdit->clear();
}

void AnnotationDock::updateInputState(bool enabled)
{
	annotationEdit->setReadOnly(!enabled);
	saveChapterMarkerButton->setEnabled(enabled);

	if (!enabled) {
		annotationEdit->setText(obs_module_text("AnnotationMainError"));
	} else {
		annotationEdit->clear();
	}
}
