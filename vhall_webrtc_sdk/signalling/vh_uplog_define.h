#pragma once

/* publish */
// 182005 调用推流接口
#define invokePublish "182005"

// 184001 链接信令服务器 链接信令服务器失败
#define signallingConnectFailed "184001"
// 184002 调用推流接口失败
#define invokePublishFailed "184002"
// 184003 发送pulish信令返回失败
#define publishFailed "184003"
// 184004	本地流peerconnection链接失败
#define localPeerConnectionFailed "184004"
// 184005	服务器删除本地流
#define serverDeleteLocalStream "184005"
// 184006	调用订阅接口失败
#define invokeSubscribeFailed "184006"
// 184007	发送subscribe信令返回失败	
#define subscribeFailed "184007"
// 184008	远端流peerconnection失败
#define RemotePeerConnectionFailed "184008"
// 184009 调用取消推流接口失败
#define invokeUnpublishFailed "184009"
// 184010	发送unpublish信令返回失败
#define unpublishFailed "184010"
// 184011	调用取消订阅接口失败
#define invokeUnsubscribeFailed "184011"
// 184012	发送unsubscribe信令返回失败
#define unsubscribeReturnFailed "184012"
// 184013	信令服务器断开链接
#define signallingDisconnected "184013"
// 184014	本地流未找到
#define LocalStreamNotExists "184014"
// 摄像头被移除
#define ulCamerLost "184015"
// 音频设备被移除
#define ulAudioDeviceLost "184016"

// 182006 向服务端发送publish信令
#define sendPublishSignalling "182006"
// 182007 pulish信令返回成功
#define publishSuccess "182007"

/* unpublish */
// 182022 调用取消推流接口
#define invokeUnpublish "182022"
// 182023	发送unpublish信令
#define sendUnpublishSignalling "182023"
// 182024 unpublish信令返回成功
#define unpublishSuccess "182024"
// 182027	调用断开信令链接接口
#define invokeDisconnectSignalling "182027"

/* subscribe */
// 182013	调用订阅接口
#define invokeSubscribe "182013"

// 182014	向服务器发送subscribe信令
#define sendSubscribeSignalling "182014"
// 182015	subscribe信令返回成功
#define subscribeSuccess "182015"

/* Mix stream */
// 182201	调用配置混流（configRoomBroadCast）接口并发送信令
#define invokeConfigRoomBroadCast "182201"
// 182202	配置混流信令返回成功
#define configRoomBroadCastSuccess "182202"
// 184201	配置混流信令返回失败
#define configRoomBroadCastFailed "184201"
// 182203	调用配置混流模式（setMixLayoutMode）接口并发送信令
#define invokeSetMixLayoutMode "182203"
// 182204	配置混流模式信令返回成功
#define setMixLayoutSuccess "182204"
// 184202	配置混流模式信令返回失败
#define setMixLayoutFailed "184202"
// 182205	调用配置混流主屏（setMixLayoutMainScreen）接口并发送信令
#define invodeSetMixLayoutMainScreen "182205"
// 182206	配置混流主屏信令返回成功
#define setMixLayoutMainScreenSuccess "182206"
// 184203	配置混流主屏信令返回失败
#define setMixLayoutMainScreenFailed "184203"
// 182207	调用开始混流（startRoomBroadCast）接口并发送信令
#define invokeStartRoomBroadCast "182207"
// 182208	开始混流信令返回成功
#define startRoomBroadCastSuccess "182208"
// 184204	开始混流信令返回失败
#define startRoomBroadCastFailed "184204"
// 182209	调用停止混流（stopRoomBroadCast）接口并发送信令
#define invokeStopRoomBroadCast "182209"
// 182210	停止混流信令返回成功
#define stopRoomBroadCastSuccess "182210"
// 184205	停止混流信令返回失败
#define stopRoomBroadCastFailed "184205"

/* unsubscribe */
// 182025	调用取消订阅接口
#define invokeUnsubscribe "182025"
// 182026	发送unsubscribe信令
#define sendUnsubscribeSignalling "182026"
// 182029	发送unsubscribe信令返回成功
#define unsubscribeSuccess "182029"
// 本地流初始化配置参数
#define ulInitStreamParam "182030"

/* stream */
// 184501	初始化本地流失败
#define initLocalStreamFailed "184501"
// 184502	监听音频音量失败
#define listenStreamVolumeFailed "184502"
// 184503	stream-ended触发时 reserved
// 184504	stream-stunk触发时	 reserved
//	stream - failed触发时
#define ulStreamFailed "184505"
// 182021	有新流添加到房间
#define newStreamAddedToRoom "182021"

// 182501	初始化本地流
#define initLocalStream "182501"
// 182502	播放流(开始渲染)
#define playStream "182502"
// 182503	停止播放流（停止渲染）
#define stopPlayStream "182503"
// 182504	调用音频静音接口
#define invokeMuteAudio "182504"
// 182505	调用视频静音接口
#define invokeMuteVideo "182505"
// 182506	上报视频截图
#define uploadScreenShot "182506"
// 182507	上报音频音量
#define uploadAudioVolume "182507"
// 508 设置变声
#define ulSetVoiceChange "182508"
// 切换扬声器
#define ulSetSpeaker "182509"
// 切换麦克风/桌面音频
#define ulSetMicroPhone "182510"
// 切换摄像头/图片
#define ulSetVideoCapDev "182511"
// 视频res/fps自适应调节
#define ulVideoAdapted "182511"
//	视频宽 / 高 / 帧率自适应调节
#define ulVideoAdapted "182512" // todo
//	关闭互动流
#define ulCloseStream "182513"
//	推流ICE状态数据
#define ulLocalIceState "182514"
//	订阅ICE状态数据
#define ulRemoteIceState "182515"

// 182701	上报麦克风设备列表
#define ulMicroPhoneList "182701"
// 182702	上报扬声器设备列表
#define ulSpeakerList "182702"
// 182703	上报摄像头设备 / 支持分辨率列表
#define ulCameraList "182703"

/* local stream */
// 182008	创建本地peerconnection
#define createLocalPeerconnection "182008"
// 182009	本地流发送offer
#define sendLocalOffer "182009"
// 182010	本地流接收并处理answer
#define ProcessAnswer "182010"
// 182011	本地流发送candidate
#define sendLocalCandidate "182011"
// 182012	本地流接收ready，推送成功
#define localStreamReady "182012"

/* remote stream */
// 182016	创建远端流peerconnection
#define createRemotePeerconnection "182016"
// 182017	发送远端流offer
#define sendRemoteOffer "182017"
// 182018	接收并处理远端流answer
#define processRemoteAnswer "182018"
// 182019	发送远端流candidate
#define sendRemoteCandidate "182019"
// 182020	接收远端流ready，订阅远端流成功
#define RemoteStreamReady "182020"

// 182028	服务器删除远端流（远端流离开房间）
#define streamDeleted "182028"

// getstats质量统计信息
#define ulQualityStats "182100"

#define connectSignalling "182001"
#define disconnectSignalling "182002"