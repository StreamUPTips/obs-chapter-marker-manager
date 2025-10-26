#include "annotation-dock.hpp"
#include "chapter-marker-dock.hpp"
#include "constants.hpp"
#include <obs-frontend-api.h>
#include <obs-module.h>
#include <QFile>
#include <QTextStream>

#define QT_TO_UTF8(str) str.toUtf8().constData()

AnnotationDock::AnnotationDock(ChapterMarkerDock *chapterDock, QWidget *parent)
	: QFrame(parent),
	  annotationEdit(new QTextEdit(this)),
	  feedbackLabel(new QLabel("", this)),
	  saveAnnotationButton(new QPushButton(obs_module_text("SaveAnnotationText"), this)),
	  chapterDock(chapterDock)
{
	setupUI();
	setupConnections();

	if (chapterDock) {
		updateInputState(chapterDock->exportChaptersToFileEnabled);
	}
}

AnnotationDock::~AnnotationDock()
{
	// Qt parent-child relationship handles cleanup
}

void AnnotationDock::setupUI()
{
	QVBoxLayout *mainLayout = new QVBoxLayout(this);
	mainLayout->addWidget(annotationEdit);

	saveAnnotationButton->setToolTip(obs_module_text("SaveAnnotationButtonToolTip"));
	mainLayout->addWidget(saveAnnotationButton);

	feedbackLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	feedbackLabel->setWordWrap(true);
	feedbackLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	mainLayout->addWidget(feedbackLabel);

	setLayout(mainLayout);
}

void AnnotationDock::setupConnections()
{
	connect(saveAnnotationButton, &QPushButton::clicked, this, &AnnotationDock::onSaveAnnotationButton);

	feedbackTimer.setInterval(Constants::FEEDBACK_TIMER_INTERVAL);
	feedbackTimer.setSingleShot(true);
	connect(&feedbackTimer, &QTimer::timeout, [this]() { feedbackLabel->setText(""); });
}

void AnnotationDock::onSaveAnnotationButton()
{
	if (!chapterDock) {
		return;
	}

	const QString annotationText = annotationEdit->toPlainText();
	const QString timestamp = chapterDock->getCurrentRecordingTime();

	chapterDock->writeAnnotationToFiles(annotationText, timestamp, obs_module_text("SourceManual"));
}

void AnnotationDock::updateInputState(bool enabled)
{
	annotationEdit->setReadOnly(!enabled);
	saveAnnotationButton->setEnabled(enabled);

	if (!enabled) {
		annotationEdit->setText(obs_module_text("AnnotationMainError"));
	} else {
		annotationEdit->clear();
	}
}
