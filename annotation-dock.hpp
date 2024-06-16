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

public slots:
	void onSaveAnnotationButton();

private:
	void setupUI();
	void setupConnections();

	QTextEdit *annotationEdit;
	QPushButton *saveChapterMarkerButton;
	QLabel *feedbackLabel;
	QTimer feedbackTimer;

	ChapterMarkerDock *chapterDock; // Pointer to ChapterMarkerDock
};

#endif // ANNOTATION_DOCK_HPP
