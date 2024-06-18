#pragma once

#ifndef ANNOTATION_DOCK_HPP
#define ANNOTATION_DOCK_HPP

#include <QFrame>
#include <QTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>
#include <QTimer>
#include <obs-frontend-api.h>


class ChapterMarkerDock; // Forward declaration

class AnnotationDock : public QFrame {
	Q_OBJECT

public:
	AnnotationDock(ChapterMarkerDock *chapterDock,
		       QWidget *parent = nullptr);
	~AnnotationDock();

	void updateInputState(bool enabled);

	QTextEdit *annotationEdit;
	QLabel *feedbackLabel;
	QTimer feedbackTimer;

public slots:
	void onSaveAnnotationButton();

private:
	void setupUI();
	void setupConnections();

	QPushButton *saveChapterMarkerButton;

	ChapterMarkerDock *chapterDock;
};

#endif // ANNOTATION_DOCK_HPP
