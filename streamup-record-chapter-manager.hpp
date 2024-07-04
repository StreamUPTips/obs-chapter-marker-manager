#pragma once

#ifndef STREAMUP_RECORD_CHAPTER_MANAGER_HPP
#define STREAMUP_RECORD_CHAPTER_MANAGER_HPP

#include <obs.h>
#include <obs-frontend-api.h>
#include <obs-source.h>
#include <QString>

obs_data_t *SaveLoadSettingsCallback(obs_data_t *save_data, bool saving);

QString GetCurrentChapterName();
extern QString currentChapterName;

void EmitWebSocketEvent(const char *event_type, obs_data_t *data);

#endif // STREAMUP_RECORD_CHAPTER_MANAGER_HPP
