#include "annotation-dock.hpp"
#include "chapter-marker-dock.hpp"
#include <QFile>
#include <QTextStream>
#include <obs-module.h>
#include <obs-frontend-api.h>

#define QT_TO_UTF8(str) str.toUtf8().constData()

AnnotationDock::AnnotationDock(ChapterMarkerDock *chapterDock, QWidget *parent)
	: QFrame(parent),
	  annotationEdit(new QTextEdit(this)),
	  feedbackLabel(new QLabel("", this)),
	  saveChapterMarkerButton(new QPushButton(obs_module_text("SaveAnnotationText"), this)),
	  chapterDock(chapterDock)
{
	setupUI();
	setupConnections();
	updateInputState(chapterDock->exportChaptersToFileEnabled);
}

AnnotationDock::~AnnotationDock() {}

void AnnotationDock::setupUI()
{
	QVBoxLayout *mainLayout = new QVBoxLayout(this);
	mainLayout->addWidget(annotationEdit);
	saveChapterMarkerButton->setToolTip(obs_module_text("SaveAnnotationButtonToolTip"));
	mainLayout->addWidget(saveChapterMarkerButton);
	feedbackLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	feedbackLabel->setWordWrap(true);
	feedbackLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	mainLayout->addWidget(feedbackLabel);
	setLayout(mainLayout);
}

void AnnotationDock::setupConnections()
{
	connect(saveChapterMarkerButton, &QPushButton::clicked, this, &AnnotationDock::onSaveAnnotationButton);

	feedbackTimer.setInterval(5000);
	feedbackTimer.setSingleShot(true);
	connect(&feedbackTimer, &QTimer::timeout, [this]() { feedbackLabel->setText(""); });
}

void AnnotationDock::onSaveAnnotationButton()
{
	QString annotationText = annotationEdit->toPlainText();

	QString timestamp = chapterDock->getCurrentRecordingTime();
	chapterDock->writeAnnotationToFiles(annotationText, timestamp, obs_module_text("SourceManual"));
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
