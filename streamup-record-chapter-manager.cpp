#include "streamup-record-chapter-manager.hpp"
#include "chapter-marker-dock.hpp"
#include "version.h"
#include <obs.h>
#include <obs-data.h>
#include <obs-encoder.h>
#include <obs-frontend-api.h>
#include <obs-module.h>
#include <QMainWindow>
#include <QDockWidget>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QFileInfo>
#include <util/platform.h>

#define QT_UTF8(str) QString::fromUtf8(str)
#define QT_TO_UTF8(str) str.toUtf8().constData()

OBS_DECLARE_MODULE()
OBS_MODULE_AUTHOR("Andilippi");
OBS_MODULE_USE_DEFAULT_LOCALE("streamup-record-chapter-manager", "en-US")

static ChapterMarkerDock *dock_widget = nullptr;

static void LoadChapterMarkerDock()
{
	const auto main_window =
		static_cast<QMainWindow *>(obs_frontend_get_main_window());
	obs_frontend_push_ui_translation(obs_module_get_string);

	if (!dock_widget) {
		dock_widget = new ChapterMarkerDock(main_window);

		const QString title = QString::fromUtf8(
			obs_module_text("StreamUP Chapter Marker"));
		const auto name = "ChapterMarkerDock";

#if LIBOBS_API_VER >= MAKE_SEMANTIC_VERSION(30, 0, 0)
		obs_frontend_add_dock_by_id(name, QT_TO_UTF8(title),
					    dock_widget);
#else
		auto dock = new QDockWidget(main_window);
		dock->setObjectName(name);
		dock->setWindowTitle(title);
		dock->setWidget(dock_widget);
		dock->setFeatures(QDockWidget::DockWidgetMovable |
				  QDockWidget::DockWidgetFloatable);
		dock->setFloating(true);
		dock->hide();
		obs_frontend_add_dock(dock);
#endif

		obs_frontend_pop_ui_translation();
	}
}

static void createChapterFile()
{
	if (!dock_widget || !dock_widget->exportChaptersEnabled) {
		return;
	}

	obs_output_t *output = obs_frontend_get_recording_output();
	if (!output) {
		blog(LOG_ERROR,
		     "[StreamUP Record Chapter Manager] Could not get the recording output.");
		return;
	}

	obs_data_t *settings = obs_output_get_settings(output);
	if (!settings) {
		blog(LOG_ERROR,
		     "[StreamUP Record Chapter Manager] Could not get the recording output settings.");
		obs_output_release(output);
		return;
	}

	const char *recording_path = obs_data_get_string(settings, "path");
	if (!recording_path || strlen(recording_path) == 0) {
		blog(LOG_ERROR,
		     "[StreamUP Record Chapter Manager] Could not get the recording output path.");
		obs_data_release(settings);
		obs_output_release(output);
		return;
	}

	QString outputPath = QString::fromUtf8(recording_path);
	obs_data_release(settings);
	obs_output_release(output);

	blog(LOG_INFO, "[StreamUP Record Chapter Manager] Recording path: %s",
	     QT_TO_UTF8(outputPath));

	QFileInfo fileInfo(outputPath);
	QString baseName = fileInfo.completeBaseName();
	QString chapterFilePath =
		fileInfo.absolutePath() + "/" + baseName + "_chapters.txt";

	QFile file(chapterFilePath);
	if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
		QTextStream out(&file);
		out << "Chapter Markers for " << baseName << "\n";
		file.close();
		blog(LOG_INFO,
		     "[StreamUP Record Chapter Manager] Created chapter file: %s",
		     QT_TO_UTF8(chapterFilePath));

		dock_widget->setChapterFilePath(chapterFilePath);
		QString timestamp = dock_widget->getCurrentRecordingTime();
		dock_widget->writeChapterToFile("Start", timestamp,
						"Recording");
		dock_widget->addChapterMarker("Start", "Recording");
	} else {
		blog(LOG_ERROR,
		     "[StreamUP Record Chapter Manager] Failed to create chapter file: %s",
		     QT_TO_UTF8(chapterFilePath));
	}
}

static void on_frontend_event(enum obs_frontend_event event, void *)
{
	switch (event) {
	case OBS_FRONTEND_EVENT_RECORDING_STARTED:
		if (dock_widget) {
			dock_widget->updateCurrentChapterLabel("Start");
			createChapterFile();
		}
		break;
	case OBS_FRONTEND_EVENT_RECORDING_STOPPED:
		if (dock_widget) {
			dock_widget->setNoRecordingActive();
			dock_widget->showFeedbackMessage("Recording finished.",
							 false);
		}
		break;
	default:
		break;
	}
}

//--------------------HOTKEY HANDLERS--------------------


//--------------------MENU HELPERS--------------------
obs_data_t *SaveLoadSettingsCallback(obs_data_t *save_data, bool saving)
{
	char *configPath = obs_module_config_path("configs.json");
	obs_data_t *data = nullptr;

	blog(LOG_INFO, "[StreamUP Record Chapter Manager] Config path: %s",
	     configPath);

	if (saving) {
		blog(LOG_INFO,
		     "[StreamUP Record Chapter Manager] Saving settings...");
		if (obs_data_save_json(save_data, configPath)) {
			blog(LOG_INFO,
			     "[StreamUP Record Chapter Manager] Settings saved to %s",
			     configPath);
		} else {
			blog(LOG_WARNING,
			     "[StreamUP Record Chapter Manager] Failed to save settings to file.");
		}
	} else {
		blog(LOG_INFO,
		     "[StreamUP Record Chapter Manager] Loading settings...");
		data = obs_data_create_from_json_file(configPath);

		if (!data) {
			blog(LOG_INFO,
			     "[StreamUP Record Chapter Manager] Settings not found. Creating default settings...");
			os_mkdirs(obs_module_config_path(""));
			data = obs_data_create();
			if (obs_data_save_json(data, configPath)) {
				blog(LOG_INFO,
				     "[StreamUP Record Chapter Manager] Default settings saved to %s",
				     configPath);
			} else {
				blog(LOG_WARNING,
				     "[StreamUP Record Chapter Manager] Failed to save default settings to file.");
			}
		} else {
			blog(LOG_INFO,
			     "[StreamUP Record Chapter Manager] Settings loaded successfully from %s",
			     configPath);
		}
	}

	bfree(configPath);
	return data;
}

//--------------------STARTUP COMMANDS--------------------
bool obs_module_load()
{
	blog(LOG_INFO, "[StreamUP Record Chapter Manager] loaded version %s",
	     PROJECT_VERSION);

	obs_frontend_add_event_callback(on_frontend_event, nullptr);

	LoadChapterMarkerDock();

	if (dock_widget) {
		dock_widget->loadSettings();
	}

	return true;
}

void obs_module_post_load(void)
{
	// Load settings
	obs_data_t *settings = SaveLoadSettingsCallback(nullptr, false);

	obs_data_release(settings);
}

//--------------------EXIT COMMANDS--------------------
void obs_module_unload()
{

}

MODULE_EXPORT const char *obs_module_description(void)
{
	return obs_module_text("Description");
}

MODULE_EXPORT const char *obs_module_name(void)
{
	return obs_module_text("StreamUPRecordChapterManager");
}
