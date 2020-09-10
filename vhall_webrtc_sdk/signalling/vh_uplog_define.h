#pragma once

/* publish */
// 182005 ���������ӿ�
#define invokePublish "182005"

// 184001 ������������� �������������ʧ��
#define signallingConnectFailed "184001"
// 184002 ���������ӿ�ʧ��
#define invokePublishFailed "184002"
// 184003 ����pulish�����ʧ��
#define publishFailed "184003"
// 184004	������peerconnection����ʧ��
#define localPeerConnectionFailed "184004"
// 184005	������ɾ��������
#define serverDeleteLocalStream "184005"
// 184006	���ö��Ľӿ�ʧ��
#define invokeSubscribeFailed "184006"
// 184007	����subscribe�����ʧ��	
#define subscribeFailed "184007"
// 184008	Զ����peerconnectionʧ��
#define RemotePeerConnectionFailed "184008"
// 184009 ����ȡ�������ӿ�ʧ��
#define invokeUnpublishFailed "184009"
// 184010	����unpublish�����ʧ��
#define unpublishFailed "184010"
// 184011	����ȡ�����Ľӿ�ʧ��
#define invokeUnsubscribeFailed "184011"
// 184012	����unsubscribe�����ʧ��
#define unsubscribeReturnFailed "184012"
// 184013	����������Ͽ�����
#define signallingDisconnected "184013"
// 184014	������δ�ҵ�
#define LocalStreamNotExists "184014"
// ����ͷ���Ƴ�
#define ulCamerLost "184015"
// ��Ƶ�豸���Ƴ�
#define ulAudioDeviceLost "184016"

// 182006 �����˷���publish����
#define sendPublishSignalling "182006"
// 182007 pulish����سɹ�
#define publishSuccess "182007"

/* unpublish */
// 182022 ����ȡ�������ӿ�
#define invokeUnpublish "182022"
// 182023	����unpublish����
#define sendUnpublishSignalling "182023"
// 182024 unpublish����سɹ�
#define unpublishSuccess "182024"
// 182027	���öϿ��������ӽӿ�
#define invokeDisconnectSignalling "182027"

/* subscribe */
// 182013	���ö��Ľӿ�
#define invokeSubscribe "182013"

// 182014	�����������subscribe����
#define sendSubscribeSignalling "182014"
// 182015	subscribe����سɹ�
#define subscribeSuccess "182015"

/* Mix stream */
// 182201	�������û�����configRoomBroadCast���ӿڲ���������
#define invokeConfigRoomBroadCast "182201"
// 182202	���û�������سɹ�
#define configRoomBroadCastSuccess "182202"
// 184201	���û��������ʧ��
#define configRoomBroadCastFailed "184201"
// 182203	�������û���ģʽ��setMixLayoutMode���ӿڲ���������
#define invokeSetMixLayoutMode "182203"
// 182204	���û���ģʽ����سɹ�
#define setMixLayoutSuccess "182204"
// 184202	���û���ģʽ�����ʧ��
#define setMixLayoutFailed "184202"
// 182205	�������û���������setMixLayoutMainScreen���ӿڲ���������
#define invodeSetMixLayoutMainScreen "182205"
// 182206	���û�����������سɹ�
#define setMixLayoutMainScreenSuccess "182206"
// 184203	���û������������ʧ��
#define setMixLayoutMainScreenFailed "184203"
// 182207	���ÿ�ʼ������startRoomBroadCast���ӿڲ���������
#define invokeStartRoomBroadCast "182207"
// 182208	��ʼ��������سɹ�
#define startRoomBroadCastSuccess "182208"
// 184204	��ʼ���������ʧ��
#define startRoomBroadCastFailed "184204"
// 182209	����ֹͣ������stopRoomBroadCast���ӿڲ���������
#define invokeStopRoomBroadCast "182209"
// 182210	ֹͣ��������سɹ�
#define stopRoomBroadCastSuccess "182210"
// 184205	ֹͣ���������ʧ��
#define stopRoomBroadCastFailed "184205"

/* unsubscribe */
// 182025	����ȡ�����Ľӿ�
#define invokeUnsubscribe "182025"
// 182026	����unsubscribe����
#define sendUnsubscribeSignalling "182026"
// 182029	����unsubscribe����سɹ�
#define unsubscribeSuccess "182029"
// ��������ʼ�����ò���
#define ulInitStreamParam "182030"

/* stream */
// 184501	��ʼ��������ʧ��
#define initLocalStreamFailed "184501"
// 184502	������Ƶ����ʧ��
#define listenStreamVolumeFailed "184502"
// 184503	stream-ended����ʱ reserved
// 184504	stream-stunk����ʱ	 reserved
//	stream - failed����ʱ
#define ulStreamFailed "184505"
// 182021	��������ӵ�����
#define newStreamAddedToRoom "182021"

// 182501	��ʼ��������
#define initLocalStream "182501"
// 182502	������(��ʼ��Ⱦ)
#define playStream "182502"
// 182503	ֹͣ��������ֹͣ��Ⱦ��
#define stopPlayStream "182503"
// 182504	������Ƶ�����ӿ�
#define invokeMuteAudio "182504"
// 182505	������Ƶ�����ӿ�
#define invokeMuteVideo "182505"
// 182506	�ϱ���Ƶ��ͼ
#define uploadScreenShot "182506"
// 182507	�ϱ���Ƶ����
#define uploadAudioVolume "182507"
// 508 ���ñ���
#define ulSetVoiceChange "182508"
// �л�������
#define ulSetSpeaker "182509"
// �л���˷�/������Ƶ
#define ulSetMicroPhone "182510"
// �л�����ͷ/ͼƬ
#define ulSetVideoCapDev "182511"
// ��Ƶres/fps����Ӧ����
#define ulVideoAdapted "182511"
//	��Ƶ�� / �� / ֡������Ӧ����
#define ulVideoAdapted "182512" // todo
//	�رջ�����
#define ulCloseStream "182513"
//	����ICE״̬����
#define ulLocalIceState "182514"
//	����ICE״̬����
#define ulRemoteIceState "182515"

// 182701	�ϱ���˷��豸�б�
#define ulMicroPhoneList "182701"
// 182702	�ϱ��������豸�б�
#define ulSpeakerList "182702"
// 182703	�ϱ�����ͷ�豸 / ֧�ֱַ����б�
#define ulCameraList "182703"

/* local stream */
// 182008	��������peerconnection
#define createLocalPeerconnection "182008"
// 182009	����������offer
#define sendLocalOffer "182009"
// 182010	���������ղ�����answer
#define ProcessAnswer "182010"
// 182011	����������candidate
#define sendLocalCandidate "182011"
// 182012	����������ready�����ͳɹ�
#define localStreamReady "182012"

/* remote stream */
// 182016	����Զ����peerconnection
#define createRemotePeerconnection "182016"
// 182017	����Զ����offer
#define sendRemoteOffer "182017"
// 182018	���ղ�����Զ����answer
#define processRemoteAnswer "182018"
// 182019	����Զ����candidate
#define sendRemoteCandidate "182019"
// 182020	����Զ����ready������Զ�����ɹ�
#define RemoteStreamReady "182020"

// 182028	������ɾ��Զ������Զ�����뿪���䣩
#define streamDeleted "182028"

// getstats����ͳ����Ϣ
#define ulQualityStats "182100"

#define connectSignalling "182001"
#define disconnectSignalling "182002"