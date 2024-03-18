#pragma once

#include "miniaudio.h"

void sound_at_end_callback(void* pUserData, ma_sound* pSound);
void sound_init_notification_callback(ma_async_notification* pNotification);