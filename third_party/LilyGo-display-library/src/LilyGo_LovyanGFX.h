#pragma once

#if defined(LILYGO_LGFX_USE_ALL) || defined(LILYGO_LGFX_USE_T_DISPLAY)
#include "LilyGo_T_Display.h"
#endif
#if defined(LILYGO_LGFX_USE_ALL) || defined(LILYGO_LGFX_USE_T_DISPLAY_S3)
#include "LilyGo_T_Display_S3.h"
#endif
#if defined(LILYGO_LGFX_USE_ALL) || defined(LILYGO_LGFX_USE_T_CIRCLE_S3)
#include "LilyGo_T_Circle_S3.h"
#endif
#if defined(LILYGO_LGFX_USE_ALL) || defined(LILYGO_LGFX_USE_T_CIRCLE)
#include "LilyGo_T_Circle.h"
#endif
#if defined(LILYGO_LGFX_USE_ALL) || defined(LILYGO_LGFX_USE_T_DISPLAY_S3_AMOLED)
#include "LilyGo_T_Display_S3_AMOLED.h"
#endif
#if defined(LILYGO_LGFX_USE_ALL) || defined(LILYGO_LGFX_USE_T4_S3)
#include "LilyGo_T4_S3.h"
#endif
#if defined(LILYGO_LGFX_USE_ALL) || defined(LILYGO_LGFX_USE_T_DECK)
#include "LilyGo_T_Deck.h"
#endif
#if defined(LILYGO_LGFX_USE_ALL) || defined(LILYGO_LGFX_USE_T_DECK_PLUS)
#include "LilyGo_T_Deck_Plus.h"
#endif
#if defined(LILYGO_LGFX_USE_ALL) || defined(LILYGO_LGFX_USE_T_EMBED_CC1101)
#include "LilyGo_T_Embed_CC1101.h"
#endif
#if defined(LILYGO_LGFX_USE_ALL) || defined(LILYGO_LGFX_USE_T_EMBED)
#include "LilyGo_T_Embed.h"
#endif
#if defined(LILYGO_LGFX_USE_ALL) || defined(LILYGO_LGFX_USE_T_DISPLAY_BAR)
#include "LilyGo_T_Display_Bar.h"
#endif
#if defined(LILYGO_LGFX_USE_ALL) || defined(LILYGO_LGFX_USE_T_DISPLAY_KEYBOARD)
#include "LilyGo_T_Display_Keyboard.h"
#endif
#if defined(LILYGO_LGFX_USE_ALL) || defined(LILYGO_LGFX_USE_T_DONGLE_S3)
#include "LilyGo_T_Dongle_S3.h"
#endif
#if defined(LILYGO_LGFX_USE_ALL) || defined(LILYGO_LGFX_USE_T_DONGLE_C5)
#include "LilyGo_T_Dongle_C5.h"
#endif
#if defined(LILYGO_LGFX_USE_ALL) || defined(LILYGO_LGFX_USE_T_ENCODER)
#include "LilyGo_T_Encoder.h"
#endif
#if defined(LILYGO_LGFX_USE_ALL) || defined(LILYGO_LGFX_USE_T_ENCODER_PRO)
#include "LilyGo_T_Encoder_Pro.h"
#endif
#if defined(LILYGO_LGFX_USE_ALL) || defined(LILYGO_LGFX_USE_T_CONNECT_PRO)
#include "LilyGo_T_Connect_Pro.h"
#endif
#if defined(LILYGO_LGFX_USE_ALL) || defined(LILYGO_LGFX_USE_T_CAMERA_PLUS_S3)
#include "LilyGo_T_Camera_Plus_S3.h"
#endif
#if defined(LILYGO_LGFX_USE_ALL) || defined(LILYGO_LGFX_USE_T_DISPLAY_S3_PRO)
#include "LilyGo_T_Display_S3_Pro.h"
#endif

#if defined(LILYGO_LGFX_USE_ALL) || defined(LILYGO_LGFX_USE_T_DISPLAY)
using LilyGoTDisplay = LilyGo_T_Display;
#endif
#if defined(LILYGO_LGFX_USE_ALL) || defined(LILYGO_LGFX_USE_T_DISPLAY_S3)
using LilyGoTDisplayS3 = LilyGo_T_Display_S3;
#endif
#if defined(LILYGO_LGFX_USE_ALL) || defined(LILYGO_LGFX_USE_T_CIRCLE_S3)
using LilyGoTCircleS3 = LilyGo_T_Circle_S3;
#endif
#if defined(LILYGO_LGFX_USE_ALL) || defined(LILYGO_LGFX_USE_T_CIRCLE)
using LilyGoTCircle = LilyGo_T_Circle;
#endif
#if defined(LILYGO_LGFX_USE_ALL) || defined(LILYGO_LGFX_USE_T_DISPLAY_S3_AMOLED)
using LilyGoTDisplayS3AMOLED = LilyGo_T_Display_S3_AMOLED;
#endif
#if defined(LILYGO_LGFX_USE_ALL) || defined(LILYGO_LGFX_USE_T4_S3)
using LilyGoT4S3 = LilyGo_T4_S3;
#endif
#if defined(LILYGO_LGFX_USE_ALL) || defined(LILYGO_LGFX_USE_T_DECK)
using LilyGoTDeck = LilyGo_T_Deck;
#endif
#if defined(LILYGO_LGFX_USE_ALL) || defined(LILYGO_LGFX_USE_T_DECK_PLUS)
using LilyGoTDeckPlus = LilyGo_T_Deck_Plus;
#endif
#if defined(LILYGO_LGFX_USE_ALL) || defined(LILYGO_LGFX_USE_T_EMBED_CC1101)
using LilyGoTEmbedCC1101 = LilyGo_T_Embed_CC1101;
#endif
#if defined(LILYGO_LGFX_USE_ALL) || defined(LILYGO_LGFX_USE_T_EMBED)
using LilyGoTEmbed = LilyGo_T_Embed;
#endif
#if defined(LILYGO_LGFX_USE_ALL) || defined(LILYGO_LGFX_USE_T_DISPLAY_BAR)
using LilyGoTDisplayBar = LilyGo_T_Display_Bar;
#endif
#if defined(LILYGO_LGFX_USE_ALL) || defined(LILYGO_LGFX_USE_T_DISPLAY_KEYBOARD)
using LilyGoTDisplayKeyboard = LilyGo_T_Display_Keyboard;
#endif
#if defined(LILYGO_LGFX_USE_ALL) || defined(LILYGO_LGFX_USE_T_DONGLE_S3)
using LilyGoTDongleS3 = LilyGo_T_Dongle_S3;
#endif
#if defined(LILYGO_LGFX_USE_ALL) || defined(LILYGO_LGFX_USE_T_DONGLE_C5)
using LilyGoTDongleC5 = LilyGo_T_Dongle_C5;
#endif
#if defined(LILYGO_LGFX_USE_ALL) || defined(LILYGO_LGFX_USE_T_ENCODER)
using LilyGoTEncoder = LilyGo_T_Encoder;
#endif
#if defined(LILYGO_LGFX_USE_ALL) || defined(LILYGO_LGFX_USE_T_ENCODER_PRO)
using LilyGoTEncoderPro = LilyGo_T_Encoder_Pro;
#endif
#if defined(LILYGO_LGFX_USE_ALL) || defined(LILYGO_LGFX_USE_T_CONNECT_PRO)
using LilyGoTConnectPro = LilyGo_T_Connect_Pro;
#endif
#if defined(LILYGO_LGFX_USE_ALL) || defined(LILYGO_LGFX_USE_T_CAMERA_PLUS_S3)
using LilyGoTCameraPlusS3 = LilyGo_T_Camera_Plus_S3;
#endif
#if defined(LILYGO_LGFX_USE_ALL) || defined(LILYGO_LGFX_USE_T_DISPLAY_S3_PRO)
using LilyGoTDisplayS3Pro = LilyGo_T_Display_S3_Pro;
#endif
