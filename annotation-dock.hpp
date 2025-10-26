#pragma once

#ifndef ANNOTATION_DOCK_HPP
#define ANNOTATION_DOCK_HPP

#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QTimer>

// Forward declarations
class ChapterMarkerDock;
class QVBoxLayout;
struct obs_frontend_event;

/**
 * @class AnnotationDock
 * @brief UI dock for adding annotations during recording
 *
 * Provides a text input interface for users to add timestamped annotations
 * that are exported alongside chapter markers.
 */
class AnnotationDock : public QFrame {
	Q_OBJECT

public:
	explicit AnnotationDock(ChapterMarkerDock *chapterDock, QWidget *parent = nullptr);
	~AnnotationDock();

	// Public interface
	void updateInputState(bool enabled);

	// Public members (accessed by ChapterMarkerDock)
	QTextEdit *annotationEdit;
	QLabel *feedbackLabel;
	QTimer feedbackTimer;

public slots:
	void onSaveAnnotationButton();

private:
	// UI setup
	void setupUI();
	void setupConnections();

	// UI components
	QPushButton *saveAnnotationButton;

	// Parent reference
	ChapterMarkerDock *chapterDock;
};

#endif // ANNOTATION_DOCK_HPP
