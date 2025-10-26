#pragma once

#ifndef STREAMUP_RECORD_CHAPTER_MANAGER_HPP
#define STREAMUP_RECORD_CHAPTER_MANAGER_HPP

#include <obs.h>
#include <obs-frontend-api.h>
#include <obs-source.h>
#include <QString>

// Forward declarations
class QString;
struct obs_data;

// Settings management
obs_data_t *SaveLoadSettingsCallback(obs_data_t *save_data, bool saving);

// Chapter name management
QString GetCurrentChapterName();
extern QString currentChapterName;

// WebSocket integration
void EmitWebSocketEvent(const char *event_type, obs_data_t *data);

// Hotkey callbacks
void AddDefaultChapterMarkerHotkey(void *data, obs_hotkey_id id, obs_hotkey_t *hotkey, bool pressed);
void AddChapterMarkerHotkey(void *data, obs_hotkey_id id, obs_hotkey_t *hotkey, bool pressed);

#endif // STREAMUP_RECORD_CHAPTER_MANAGER_HPP
