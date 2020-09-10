//
//  vhall_define.h
//  VhallSignaling
//
//  Created by ilong on 2017/12/19.
//  Copyright © 2017年 ilong. All rights reserved.
//

#ifndef vhall_define_h
#define vhall_define_h
#include <string>
#include <functional>

#ifdef __cplusplus
#define NS_VH_BEGIN                              namespace vhall {
#define NS_VH_END                                }
#define USING_NS_VH                              using namespace vhall
#define NS_VH                                    ::vhall
#else
#define NS_VH_BEGIN
#define NS_VH_END
#define USING_NS_VH
#define NS_VH
#endif

typedef std::function<void (const void * audio_data, int bits_per_sample, int sample_rate, size_t number_of_channels, size_t number_frames_perChannel)> PushAudioData;

typedef enum {
   VhallDisconnected = 0,
   VhallConnecting = 1,
   VhallConnected = 2,
   VhallReconnecting = 3,
}VhallConnectState;

typedef enum {
  Uninitialized,
  Initialized,
  Publish,
  UnPublish,
  Subscribe,
  UnSubscribe
}VhallStreamState;

/* about signalling */
//server -> client
#define SIGNALING_MESSAGE_VHALL                  "signaling_message_vhall"
#define SIGNALING_MESSAGE_PEER                   "signaling_message_peer"
#define PUBLISH_ME                               "publish_me"
#define UNPUBLISH_ME                             "unpublish_me"
#define ON_BANDWIDTH_ALERT                       "onBandwidthAlert"
#define ON_DATA_STREAM                           "onDataStream"
#define ON_UPDATE_ATTRIBUTESTREAM                "onUpdateAttributeStream"
#define ON_REMOVE_STREAM                         "onRemoveStream"
#define ON_ADD_STREAM                            "onAddStream"
#define DISCONNECT                               "disconnect"
#define CONNECTION_FAILED                        "connection_failed"
#define VHERROR                                  "error"
#define BROADCAST_MUTE_STREAM                    "broadcastMuteStream"
#define STREAM_APPLY                             "StreamApply"
#define STREAM_CONSENT                           "StreamConsent"
#define STREAM_REJECT                            "StreamReject"
#define USER_DELETED                             "userDeleted"
#define USERS_PERMISSION_CHANGED                 "usersPermissionChanged"
#define CUSTOM_SIGNALING                         "customSignalling"
#define REMOTE_USER_ENTER_ROOM                   "remoteUserEnterRoom"
#define REMOTE_USER_QUIT_ROOM                    "remoteUserQuitRoom"
#define ON_STREAM_MIXED                          "onStreamMixed"
//client -> server
#define TOKEN                                    "token"
#define RECONNECTED                              "reconnected"
#define PUBLISH                                  "publish"
#define UNPUBLISH                                "unpublish"
#define SUBSCRIBE                                "subscribe"
#define UNSUBSCRIBE                              "unsubscribe"
#define GETOVERSEA                               "getOverseas"
#define SIGNALING_MESSAGE                        "signaling_message"
#define UPDATE_STREAM_ATTRIBUTES                 "updateStreamAttributes"
#define GET_STREAM_STATS                         "getStreamStats"
#define START_RECORDER                           "startRecorder"
#define STOP_RECORDER                            "stopRecorder"
#define SEND_DATA_STREAM                         "sendDataStream"
#define CONFIG_ROOM_BROAD_CAST                   "ConfigRoomBroadCast"
#define SET_MIX_LAYOUT_MAIN_SCREEN               "SetMixLayoutMainScreen"
#define SET_MIX_LAYOUT_MODE                      "SetMixLayoutMode"
#define SET_MAX_USER_COUNT                       "setMaxUserCount"
#define SET_ROOM_MIX_CONFIG                      "SetRoomMixConfig"
#define START_ROOM_BROAD_CAST                    "StartRoomBroadCast"
#define STOP_ROOM_BROAD_CAST                     "StopRoomBroadCast"
#define SEND_MUTE_STREAM                         "sendMuteStream"
#define ADD_STREAM_TO_MIX                        "addStreamToMix"
#define DEL_STREAM_FROM_MIX                      "delStreamFromMix"

/* Event Type */
// use for room
#define ROOM_DISCONNECTED                        "room-disconnected"
#define ROOM_RECONNECTED                         "room-reconnected"
#define ROOM_CONNECTED                           "room-connected"
#define ROOM_CONNECTING                          "room-connecting" // try again
#define ROOM_RECONNECTING                        "room-reconnecting"
#define ROOM_RECOVER                             "room-recover"
#define ROOM_ERROR                               "room-error"
#define ROOM_NETWORKCHANGED                      "room-networkchanged"
#define STREAM_SUBSCRIBED                        "stream-subscribed"
#define STREAM_ADDED                             "stream-added"
#define STREAM_REMOVED                           "stream-removed"
#define STREAM_FAILED                            "stream-failed"
#define STREAM_SOURCE_LOST                       "stream-source-lost"
#define STREAM_RECONNECT                         "stream-reconnect"
#define STREAM_RECONNECT_FAILED                  "stream-reconnect-failed"
//#define STREAM_RECONNECT_SUCCESS                 "stream-reconnect-success"
#define STREAM_REPUBLISHED                       "stream-republished"
#define STREAM_READY                             "stream-ready" // publish&&subscribe
// use for stream
#define ACCESS_ACCEPTED                          "access-accepted"
#define ACCESS_DENIED                            "access-denied"
#define VIDEO_DENIED                             "video-denied"
#define VIDEO_RECEIVE_LOST                       "videoReceiveLost"
#define CAMERA_LOST                              "camera-lost"
#define VIDEO_CAPTURE_ERROR                      "video capture error stopped" /* camera状态正常但无采集数据 */

#define AUDIO_DENIED                             "audio-denied"
#define AUDIO_IN_ERROR                           "audioInDevNotAvailable"
#define AUDIO_OUT_ERROR                          "audioOutDevNotAvalable"

#define AUDIO_CAPTURE_ERROR                      "audio_capture_error"
#define AUDIO_DEVICE_PROPERTY                    "audio_device_property"
#define AUDIO_DEVICE_STATE                       "audio_device_state"
#define AUDIO_DEVICE_REMOVED                     "audio_device_removed"
#define AUDIO_DEVICE_ADDED                       "audio_device_added"
#define AUDIO_DEFAULT_DEVICE_CHANGED             "audio_default_device_changed"

// core audio operation
#define IAUDIOCLIENT_INITIALIZE                  "IAudioClientInitialize"
#define COREAUDIOCAPTUREERROR                    "CoreAudioCaptureError"


/** @def CC_DISALLOW_COPY_AND_ASSIGN(TypeName)
 * A macro to disallow the copy constructor and operator= functions.
 * This should be used in the private: declarations for a class
 */
#if defined(__GNUC__) && ((__GNUC__ >= 5) || ((__GNUG__ == 4) && (__GNUC_MINOR__ >= 4))) \
|| (defined(__clang__) && (__clang_major__ >= 3)) || (_MSC_VER >= 1800)
#define VH_DISALLOW_COPY_AND_ASSIGN(TypeName) \
TypeName(const TypeName &) = delete; \
TypeName &operator =(const TypeName &) = delete;
#else
#define VH_DISALLOW_COPY_AND_ASSIGN(TypeName) \
TypeName(const TypeName &); \
TypeName &operator =(const TypeName &);
#endif

#define VH_CALLBACK_0(__selector__,__target__, ...) std::bind(&__selector__,__target__, ##__VA_ARGS__)
#define VH_CALLBACK_1(__selector__,__target__, ...) std::bind(&__selector__,__target__, std::placeholders::_1, ##__VA_ARGS__)
#define VH_CALLBACK_2(__selector__,__target__, ...) std::bind(&__selector__,__target__, std::placeholders::_1, std::placeholders::_2, ##__VA_ARGS__)
#define VH_CALLBACK_3(__selector__,__target__, ...) std::bind(&__selector__,__target__, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, ##__VA_ARGS__)

#define VH_SAFE_DELETE(p)           do { if(p) { delete (p); (p) = nullptr; } } while(0)
#define VH_SAFE_FREE(p)             do { if(p) { free(p); (p) = nullptr; } } while(0)

#if defined (WIN32) || defined(_WIN32)
  #include <atlstr.h>
   #define __func__ __FUNCTION__
   typedef long ssize_t;
   #define VH_DLL __declspec(dllexport)
   //typedef int  size_t;
#else
   #define VH_DLL
#endif

#endif /* vhall_define_h */
