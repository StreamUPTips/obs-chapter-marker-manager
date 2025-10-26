#include "streamup-record-chapter-manager.hpp"
#include "annotation-dock.hpp"
#include "chapter-marker-dock.hpp"
#include "constants.hpp"
#include "obs-websocket-api.h"
#include "version.h"
#include <obs-data.h>
#include <obs-encoder.h>
#include <obs-frontend-api.h>
#include <obs-module.h>
#include <obs.h>
#include <util/platform.h>
#include <QDir>
#include <QDockWidget>
#include <QFile>
#include <QFileInfo>
#include <QMainWindow>
#include <QTextStream>

#define QT_UTF8(str) QString::fromUtf8(str)
#define QT_TO_UTF8(str) str.toUtf8().constData()

OBS_DECLARE_MODULE()
OBS_MODULE_AUTHOR("Andilippi");
OBS_MODULE_USE_DEFAULT_LOCALE("streamup-record-chapter-manager", "en-US")

static ChapterMarkerDock *chapterMarkerDock = nullptr;

static void LoadChapterMarkerDock()
{
	if (chapterMarkerDock) {
		return; // Already loaded
	}

	auto *main_window = static_cast<QMainWindow *>(obs_frontend_get_main_window());
	if (!main_window) {
		blog(LOG_ERROR, "[StreamUP Record Chapter Manager] Failed to get main window");
		return;
	}

	obs_frontend_push_ui_translation(obs_module_get_string);

	chapterMarkerDock = new ChapterMarkerDock(main_window);
	const QString title = QString::fromUtf8(obs_module_text("StreamUPChapterMarkerManager"));

	obs_frontend_add_dock_by_id(Constants::CHAPTER_MARKER_DOCK_ID, QT_TO_UTF8(title), chapterMarkerDock);

	obs_frontend_pop_ui_translation();
}

static void OnStartRecording()
{
	if (!chapterMarkerDock) {
		return;
	}

	if (!chapterMarkerDock->exportChaptersToFileEnabled && !chapterMarkerDock->insertChapterMarkersInVideoEnabled) {
		chapterMarkerDock->showFeedbackMessage(obs_module_text("NoExportMethod"), true);
		return;
	}

	chapterMarkerDock->createExportFiles();

	QString timestamp = chapterMarkerDock->getCurrentRecordingTime();
	chapterMarkerDock->addChapterMarker(obs_module_text("Start"), obs_module_text("Recording"));
}

static void FrontEndEventHandler(enum obs_frontend_event event, void *)
{
	switch (event) {
	case OBS_FRONTEND_EVENT_RECORDING_STARTED:
		if (chapterMarkerDock) {
			chapterMarkerDock->isFirstRunInRecording = true;
			chapterMarkerDock->updateCurrentChapterLabel(obs_module_text("Start"));
			OnStartRecording();
		}
		break;
	case OBS_FRONTEND_EVENT_RECORDING_STOPPED:
		if (chapterMarkerDock) {
			chapterMarkerDock->showFeedbackMessage(obs_module_text("ChapterMarkerNotActive"), true);
			chapterMarkerDock->showFeedbackMessage(obs_module_text("RecordingFinished"), false);
		}
		break;
	default:
		break;
	}
}

//--------------------WEBSOCKET HANDLERS--------------------
obs_websocket_vendor vendor = nullptr;

void EmitWebSocketEvent(const char *event_type, obs_data_t *data)
{
	obs_websocket_vendor_emit_event(vendor, event_type, data);
}

void WebsocketRequestSetChapterMarker(obs_data_t *request_data, obs_data_t *response_data, void *)
{
	// Check if the recording is active
	if (!obs_frontend_recording_active()) {
		// Update the response to indicate failure because recording is not active
		obs_data_set_bool(response_data, "success", false);
		obs_data_set_string(response_data, "message", obs_module_text("ChapterMarkerNotActive"));
		return;
	}

	// Retrieve chapter name and source from the request data
	const char *chapterName = obs_data_get_string(request_data, "chapterName");
	const char *chapterSource = obs_data_get_string(request_data, "chapterSource");

	// Convert to QString for easier handling
	QString qChapterName = QString::fromUtf8(chapterName ? chapterName : "");
	QString qChapterSource = QString::fromUtf8(chapterSource ? chapterSource : "");

	// If chapterName is empty, use the default chapter name logic
	if (qChapterName.isEmpty()) {
		qChapterName = chapterMarkerDock->defaultChapterName + " " + QString::number(chapterMarkerDock->chapterCount);
		chapterMarkerDock->chapterCount++;
	}

	// If chapterSource is empty, default to "WebSocket"
	if (qChapterSource.isEmpty()) {
		qChapterSource = obs_module_text("WebSocket");
	}

	// Emit the signal to add the chapter marker
	if (chapterMarkerDock) {
		emit chapterMarkerDock->addChapterMarkerSignal(qChapterName, qChapterSource);

		// Update the response to indicate success
		obs_data_set_bool(response_data, "success", true);
		obs_data_set_string(response_data, "message", obs_module_text("ChapterMarkerAdded"));
	} else {
		// Update the response to indicate failure
		obs_data_set_bool(response_data, "success", false);
		obs_data_set_string(response_data, "message", obs_module_text("ChapterMarkerNotOpen"));
	}
}

QString GetCurrentChapterName()
{
	return currentChapterName;
}

void WebsocketRequestGetCurrentChapterMarker(obs_data_t *request_data, obs_data_t *response_data, void *)
{
	UNUSED_PARAMETER(request_data);

	const QString currentChapterName = GetCurrentChapterName();

	if (!currentChapterName.isEmpty()) {
		obs_data_set_string(response_data, "chapterName", QT_TO_UTF8(currentChapterName));
		obs_data_set_bool(response_data, "success", true);
	} else {
		obs_output_t *output = obs_frontend_get_recording_output();

		if (output) {
			obs_data_set_string(response_data, "chapterName", obs_module_text("RecordingNotActive"));
			obs_data_set_bool(response_data, "success", false);
			obs_output_release(output);
		} else {
			obs_data_set_string(response_data, "chapterName", obs_module_text("ErrorGettingChapterName"));
			obs_data_set_bool(response_data, "success", false);
		}
	}
}

void WebsocketRequestSetAnnotation(obs_data_t *request_data, obs_data_t *response_data, void *)
{
	// Retrieve annotation text and source from the request data
	const char *annotationText = obs_data_get_string(request_data, "annotationText");
	const char *annotationSource = obs_data_get_string(request_data, "annotationSource");

	// Convert to QString for easier handling
	QString qAnnotationText = QString::fromUtf8(annotationText ? annotationText : "");
	QString qAnnotationSource = QString::fromUtf8(annotationSource ? annotationSource : "");

	emit chapterMarkerDock->addAnnotationSignal(qAnnotationText, qAnnotationSource);

	// Check if the recording is active
	if (!obs_frontend_recording_active()) {
		// Update the response to indicate failure because recording is not active
		obs_data_set_bool(response_data, "success", false);
		obs_data_set_string(response_data, "message", obs_module_text("AnnotationRecordingNotActive"));
		return;
	}


	// If annotationText is empty, return an error
	if (qAnnotationText.isEmpty()) {
		// Update the response to indicate failure because annotation text is empty
		obs_data_set_bool(response_data, "success", false);
		obs_data_set_string(response_data, "message", obs_module_text("AnnotationErrorTextIsEmpty"));
		return;
	}

	// If annotationSource is empty, default to "WebSocket"
	if (qAnnotationSource.isEmpty()) {
		qAnnotationSource = obs_module_text("WebSocket");
	}

	// Emit the signal to add the annotation
	if (chapterMarkerDock) {

		// Update the response to indicate success
		obs_data_set_bool(response_data, "success", true);
		obs_data_set_string(response_data, "message", obs_module_text("AnnotationAdded"));
	} else {
		// Update the response to indicate failure
		obs_data_set_bool(response_data, "success", false);
		obs_data_set_string(response_data, "message", obs_module_text("ChapterMarkerNotOpen"));
	}
}

//--------------------HOTKEY HANDLERS--------------------
obs_hotkey_id addDefaultChapterMarkerHotkey = OBS_INVALID_HOTKEY_ID;

static void SaveLoadHotkeys(obs_data_t *save_data, bool saving, void *)
{
	static bool is_first_run = true;

	if (saving) {
		// Save default chapter hotkey
		obs_data_array_t *hotkey_save_array = obs_hotkey_save(addDefaultChapterMarkerHotkey);
		obs_data_set_array(save_data, "addDefaultChapterMarkerHotkey", hotkey_save_array);
		obs_data_array_release(hotkey_save_array);

		// Save preset chapter hotkeys
		chapterMarkerDock->SaveChapterHotkeys(save_data);

		// Save preset chapters
		obs_data_array_t *chaptersArray = obs_data_array_create();
		for (const auto &chapter : chapterMarkerDock->presetChapters) {
			obs_data_t *chapterData = obs_data_create();
			obs_data_set_string(chapterData, "chapterName", QT_TO_UTF8(chapter));
			obs_data_array_push_back(chaptersArray, chapterData);
			obs_data_release(chapterData);
		}
		obs_data_set_array(save_data, "presetChapters", chaptersArray);
		obs_data_array_release(chaptersArray);

	} else {
		// Load default chapter hotkey
		obs_data_array_t *hotkey_load_array = obs_data_get_array(save_data, "addDefaultChapterMarkerHotkey");
		obs_hotkey_load(addDefaultChapterMarkerHotkey, hotkey_load_array);
		obs_data_array_release(hotkey_load_array);

		if (is_first_run) {
			// Load preset chapter hotkeys
			chapterMarkerDock->LoadChapterHotkeys(save_data);

			// Load preset chapters
			chapterMarkerDock->LoadPresetChapters(save_data);

			is_first_run = false;
		}
	}
}

void AddDefaultChapterMarkerHotkey(void *data, obs_hotkey_id id, obs_hotkey_t *hotkey, bool pressed)
{
	UNUSED_PARAMETER(data);
	UNUSED_PARAMETER(id);
	UNUSED_PARAMETER(hotkey);

	if (!pressed)
		return;
	if (!obs_frontend_recording_active()) {
		chapterMarkerDock->showFeedbackMessage(obs_module_text("ChapterMarkerNotActive"), true);

		return;
	}

	QString chapterName = chapterMarkerDock->defaultChapterName + " " + QString::number(chapterMarkerDock->chapterCount);
	chapterMarkerDock->addChapterMarker(chapterName, obs_module_text("Hotkey"));
	blog(LOG_INFO, "[StreamUP Record Chapter Manager] chapterCount: %d", chapterMarkerDock->chapterCount);

	chapterMarkerDock->chapterCount++; // Increment the chapter count
}

void AddChapterMarkerHotkey(void *data, obs_hotkey_id id, obs_hotkey_t *hotkey, bool pressed)
{
	UNUSED_PARAMETER(hotkey);

	if (!pressed)
		return;
	if (!obs_frontend_recording_active()) {
		chapterMarkerDock->showFeedbackMessage(obs_module_text("ChapterMarkerNotActive"), true);
		return;
	}

	QString chapterName;
	ChapterMarkerDock *dock = reinterpret_cast<ChapterMarkerDock *>(data);
	for (const auto &entry : dock->chapterHotkeys.toStdMap()) {
		if (entry.second == id) {
			chapterName = entry.first;
			break;
		}
	}

	if (!chapterName.isEmpty()) {
		chapterMarkerDock->addChapterMarker(chapterName, obs_module_text("PresetHotkey"));
		blog(LOG_INFO, "[StreamUP Record Chapter Manager] Added chapter marker for: %s", QT_TO_UTF8(chapterName));
	}
}

//--------------------MENU HELPERS--------------------
obs_data_t *SaveLoadSettingsCallback(obs_data_t *save_data, bool saving)
{
	char *configPath = obs_module_config_path(Constants::CONFIG_FILE_NAME);
	if (!configPath) {
		blog(LOG_ERROR, "[StreamUP Record Chapter Manager] Failed to get config path");
		return nullptr;
	}

	obs_data_t *data = nullptr;

	if (saving) {
		if (obs_data_save_json(save_data, configPath)) {
			blog(LOG_INFO, "[StreamUP Record Chapter Manager] Settings saved to %s", configPath);
		} else {
			blog(LOG_WARNING, "[StreamUP Record Chapter Manager] Failed to save settings to file.");
		}
	} else {
		data = obs_data_create_from_json_file(configPath);

		if (!data) {
			blog(LOG_INFO, "[StreamUP Record Chapter Manager] Settings not found. Creating default settings...");

			char *dirPath = obs_module_config_path("");
			if (dirPath) {
				os_mkdirs(dirPath);
				bfree(dirPath);
			}

			data = obs_data_create();
			obs_data_set_string(data, "defaultChapterName", Constants::DEFAULT_CHAPTER_NAME);

			if (obs_data_save_json(data, configPath)) {
				blog(LOG_INFO, "[StreamUP Record Chapter Manager] Default settings saved to %s", configPath);
			} else {
				blog(LOG_WARNING, "[StreamUP Record Chapter Manager] Failed to save default settings to file.");
			}

			obs_data_release(data);
			data = obs_data_create_from_json_file(configPath);
		} else {
			blog(LOG_INFO, "[StreamUP Record Chapter Manager] Settings loaded successfully from %s", configPath);
		}
	}

	bfree(configPath);
	return data;
}

//--------------------STARTUP COMMANDS--------------------
static void RegisterHotkeys()
{
	addDefaultChapterMarkerHotkey = obs_hotkey_register_frontend(Constants::HOTKEY_ADD_DEFAULT_CHAPTER,
								     obs_module_text("HotkeyAddDefaultChapterMarker"),
								     AddDefaultChapterMarkerHotkey, chapterMarkerDock);
}

static void RegisterWebsocketRequests()
{
	vendor = obs_websocket_register_vendor(Constants::VENDOR_NAME);
	if (!vendor) {
		blog(LOG_WARNING, "[StreamUP Record Chapter Manager] Failed to register WebSocket vendor");
		return;
	}

	obs_websocket_vendor_register_request(vendor, Constants::WS_REQUEST_SET_CHAPTER, WebsocketRequestSetChapterMarker,
					      nullptr);

	obs_websocket_vendor_register_request(vendor, Constants::WS_REQUEST_GET_CHAPTER,
					      WebsocketRequestGetCurrentChapterMarker, nullptr);

	obs_websocket_vendor_register_request(vendor, Constants::WS_REQUEST_SET_ANNOTATION, WebsocketRequestSetAnnotation,
					      nullptr);
}

bool (*obs_frontend_recording_add_chapter_wrapper)(const char *name) = nullptr;

bool obs_module_load()
{
	blog(LOG_INFO, "[StreamUP Record Chapter Manager] loaded version %s", PROJECT_VERSION);

	void *handle = os_dlopen("obs-frontend-api");
	obs_frontend_recording_add_chapter_wrapper =
		(bool (*)(const char *name))os_dlsym(handle, "obs_frontend_recording_add_chapter");

	RegisterHotkeys();
	RegisterWebsocketRequests();

	obs_frontend_add_save_callback(SaveLoadHotkeys, nullptr);
	obs_frontend_add_event_callback(FrontEndEventHandler, nullptr);

	LoadChapterMarkerDock();
	chapterMarkerDock->loadAnnotationDock();

	obs_data_t *settings = SaveLoadSettingsCallback(nullptr, false);

	if (settings) {
		chapterMarkerDock->LoadSettings(settings);
		obs_data_release(settings);
	}

	// Call updateInputState to initialize the state on startup
	if (chapterMarkerDock && chapterMarkerDock->annotationDock) {
		chapterMarkerDock->annotationDock->updateInputState(chapterMarkerDock->exportChaptersToFileEnabled);
	}

	return true;
}

void obs_module_post_load(void)
{
	chapterMarkerDock->refreshMainDockUI();
}

//--------------------EXIT COMMANDS--------------------
void obs_module_unload()
{
	obs_frontend_remove_event_callback(FrontEndEventHandler, nullptr);

	obs_frontend_remove_save_callback(SaveLoadHotkeys, nullptr);
	obs_hotkey_unregister(addDefaultChapterMarkerHotkey);
}

MODULE_EXPORT const char *obs_module_description(void)
{
	return obs_module_text("Description");
}

MODULE_EXPORT const char *obs_module_name(void)
{
	return obs_module_text("StreamUPRecordChapterManager");
}
