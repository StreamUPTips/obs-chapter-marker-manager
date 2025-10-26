#pragma once

#ifndef CONSTANTS_HPP
#define CONSTANTS_HPP

namespace Constants {
	// Timer intervals (milliseconds)
	constexpr int FEEDBACK_TIMER_INTERVAL = 5000;

	// UI Sizes
	constexpr int BUTTON_MIN_WIDTH = 32;
	constexpr int BUTTON_MIN_HEIGHT = 24;
	constexpr int BUTTON_MAX_WIDTH = 32;
	constexpr int BUTTON_MAX_HEIGHT = 24;
	constexpr int BUTTON_ICON_SIZE = 20;
	constexpr int INDENT_SPACING = 20;
	constexpr int LIST_HEIGHT_ROWS = 10;
	constexpr int LIST_WIDTH_PADDING = 30;

	// Default values
	constexpr const char *DEFAULT_CHAPTER_NAME = "Chapter";
	constexpr const char *DEFAULT_TIMESTAMP = "00:00:00";
	constexpr int DEFAULT_CHAPTER_COUNT = 1;

	// File extensions
	constexpr const char *TEXT_FILE_SUFFIX = "_chapters.txt";
	constexpr const char *XML_FILE_SUFFIX = "_chapters.xml";

	// Theme IDs
	constexpr const char *THEME_ERROR = "error";
	constexpr const char *THEME_GOOD = "good";
	constexpr const char *THEME_CONFIG_ICON = "configIconSmall";

	// Module names
	constexpr const char *VENDOR_NAME = "streamup-chapter-manager";
	constexpr const char *CHAPTER_MARKER_DOCK_ID = "ChapterMarkerDock";
	constexpr const char *ANNOTATION_DOCK_ID = "AnnotationDock";
	constexpr const char *CONFIG_FILE_NAME = "configs.json";

	// WebSocket events
	constexpr const char *WS_EVENT_CHAPTER_SET = "ChapterMarkerSet";
	constexpr const char *WS_EVENT_ANNOTATION_SET = "AnnotationSet";

	// WebSocket requests
	constexpr const char *WS_REQUEST_SET_CHAPTER = "setChapterMarker";
	constexpr const char *WS_REQUEST_GET_CHAPTER = "getCurrentChapterMarker";
	constexpr const char *WS_REQUEST_SET_ANNOTATION = "setAnnotation";

	// Hotkey names
	constexpr const char *HOTKEY_ADD_DEFAULT_CHAPTER = "addDefaultChapterMarker";

	// Aitum Vertical integration
	constexpr const char *AITUM_VERTICAL_PROC = "aitum_vertical_add_chapter";
	constexpr const char *AITUM_VERTICAL_PARAM = "chapter_name";
} // namespace Constants

#endif // CONSTANTS_HPP
