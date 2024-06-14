#include "annotation-dock.hpp"
#include "chapter-marker-dock.hpp"
#include <QFile>
#include <QTextStream>

#define QT_TO_UTF8(str) str.toUtf8().constData()

AnnotationDock::AnnotationDock(ChapterMarkerDock *chapterDock, QWidget *parent)
	: QFrame(parent),
	  annotationEdit(new QTextEdit(this)),
	  saveButton(new QPushButton("Save Annotation", this)),
	  feedbackLabel(new QLabel("", this)),
	  chapterDock(chapterDock)
{
	setupUI();
	setupConnections();
}

AnnotationDock::~AnnotationDock() {}

void AnnotationDock::setupUI()
{
	QVBoxLayout *mainLayout = new QVBoxLayout(this);
	mainLayout->addWidget(annotationEdit);
	mainLayout->addWidget(saveButton);
	mainLayout->addWidget(feedbackLabel);

	feedbackLabel->setStyleSheet("color: green;");
	setLayout(mainLayout);
}

void AnnotationDock::setupConnections()
{
	connect(saveButton, &QPushButton::clicked, this,
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
			"Recording is not active. Annotation not added.");
		feedbackLabel->setStyleSheet("color: red;");
		feedbackTimer.start();
		return;
	}

	QString annotationText = annotationEdit->toPlainText();
	if (annotationText.isEmpty()) {
		feedbackLabel->setText(
			"Annotation text is empty. Annotation not added.");
		feedbackLabel->setStyleSheet("color: red;");
		feedbackTimer.start();
		return;
	}

	QString timestamp = chapterDock->getCurrentRecordingTime();
	chapterDock->writeAnnotationToFiles(annotationText, timestamp, "Annotation");

	feedbackLabel->setText("Annotation saved.");
	feedbackLabel->setStyleSheet("color: green;");
	feedbackTimer.start();
	annotationEdit->clear();
}
