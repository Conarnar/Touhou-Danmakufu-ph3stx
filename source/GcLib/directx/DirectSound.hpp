#ifndef __DIRECTX_DIRECTSOUND__
#define __DIRECTX_DIRECTSOUND__

#include "../pch.h"
#include "DxConstant.hpp"

namespace directx {
	class DirectSoundManager;
	class SoundInfoPanel;
	class SoundInfo;
	class SoundDivision;
	class SoundPlayer;
	class SoundStreamingPlayer;

	class SoundPlayerWave;
	class SoundStreamingPlayerWave;
	class SoundStreamingPlayerMp3;
	class SoundStreamingPlayerOgg;

	class AcmBase;
	class AcmMp3;
	class AcmMp3Wave;

	/**********************************************************
	//DirectSoundManager
	**********************************************************/
	class DirectSoundManager {
	public:
		class SoundManageThread;
		friend SoundManageThread;
		friend SoundInfoPanel;
	public:
		enum {
			SD_VOLUME_MIN = DSBVOLUME_MIN,
			SD_VOLUME_MAX = DSBVOLUME_MAX,
		};
	private:
		static DirectSoundManager* thisBase_;
	protected:
		DSCAPS dxSoundCaps_;

		IDirectSound8* pDirectSound_;
		IDirectSoundBuffer8* pDirectSoundBuffer_;
		gstd::CriticalSection lock_;
		SoundManageThread* threadManage_;

		std::map<std::wstring, std::list<shared_ptr<SoundPlayer>>> mapPlayer_;
		std::map<int, SoundDivision*> mapDivision_;
		std::map<std::wstring, shared_ptr<SoundInfo>> mapInfo_;

		gstd::ref_count_ptr<SoundInfoPanel> panelInfo_;

		shared_ptr<SoundPlayer> _GetPlayer(const std::wstring& path);
		shared_ptr<SoundPlayer> _CreatePlayer(std::wstring path);
	public:
		DirectSoundManager();
		virtual ~DirectSoundManager();

		static DirectSoundManager* GetBase() { return thisBase_; }

		virtual bool Initialize(HWND hWnd);
		void Clear();

		const DSCAPS* GetDeviceCaps() const { return &dxSoundCaps_; }

		IDirectSound8* GetDirectSound() { return pDirectSound_; }
		gstd::CriticalSection& GetLock() { return lock_; }

		shared_ptr<SoundPlayer> GetPlayer(const std::wstring& path, bool bCreateAlways = false);
		shared_ptr<SoundPlayer> GetStreamingPlayer(const std::wstring& path);
		SoundDivision* CreateSoundDivision(int index);
		SoundDivision* GetSoundDivision(int index);
		shared_ptr<SoundInfo> GetSoundInfo(const std::wstring& path);

		void SetInfoPanel(gstd::ref_count_ptr<SoundInfoPanel> panel) { 
			gstd::Lock lock(lock_); 
			panelInfo_ = panel; 
		}

		bool AddSoundInfoFromFile(const std::wstring& path);
		std::vector<shared_ptr<SoundInfo>> GetSoundInfoList();
		void SetFadeDeleteAll();
	};

	//�t�F�[�h�C���^�t�F�[�h�A�E�g����
	//�K�v�Ȃ��Ȃ����o�b�t�@�̊J���Ȃ�
	class DirectSoundManager::SoundManageThread : public gstd::Thread, public gstd::InnerClass<DirectSoundManager> {
		friend DirectSoundManager;
	protected:
		int timeCurrent_;
		int timePrevious_;

		SoundManageThread(DirectSoundManager* manager);
		void _Run();
		void _Arrange();//�K�v�Ȃ��Ȃ����f�[�^���폜
		void _Fade();//�t�F�[�h����
	};

	/**********************************************************
	//SoundInfoPanel
	**********************************************************/
	class SoundInfoPanel : public gstd::WindowLogger::Panel {
	protected:
		struct Info {
			int address;
			std::wstring path;
			int countRef;
		};
		enum {
			ROW_ADDRESS,
			ROW_FILENAME,
			ROW_FULLPATH,
			ROW_COUNT_REFFRENCE,
		};
		gstd::WListView wndListView_;
		int timeLastUpdate_;
		int timeUpdateInterval_;

		virtual bool _AddedLogger(HWND hTab);
	public:
		SoundInfoPanel();
		void SetUpdateInterval(int time) { timeUpdateInterval_ = time; }
		virtual void LocateParts();
		virtual void Update(DirectSoundManager* soundManager);
	};

	/**********************************************************
	//SoundDivision
	//���ʂȂǂ����L���邽�߂̃N���X
	**********************************************************/
	class SoundDivision {
	public:
		enum {
			DIVISION_BGM = 0,
			DIVISION_SE,
			DIVISION_VOICE,
		};
	protected:
		double rateVolume_;//���ʊ���(0-100)
	public:
		SoundDivision();
		virtual ~SoundDivision();
		void SetVolumeRate(double rate) { rateVolume_ = rate; }
		double GetVolumeRate() { return rateVolume_; }
	};

	/**********************************************************
	//SoundInfo
	**********************************************************/
	class SoundInfo {
		friend DirectSoundManager;
	private:
		std::wstring name_;
		std::wstring title_;
		double timeLoopStart_;
		double timeLoopEnd_;
	public:
		SoundInfo() { timeLoopStart_ = 0; timeLoopEnd_ = 0; }
		virtual ~SoundInfo() {};
		std::wstring& GetName() { return name_; }
		std::wstring& GetTitle() { return title_; }
		double GetLoopStartTime() { return timeLoopStart_; }
		double GetLoopEndTime() { return timeLoopEnd_; }
	};

	/**********************************************************
	//SoundPlayer
	**********************************************************/
	class SoundPlayer {
		friend DirectSoundManager;
		friend DirectSoundManager::SoundManageThread;
	public:
		class PlayStyle;
	public:
		enum {
			FADE_DEFAULT = 20,

			INFO_FORMAT = 0,
			INFO_CHANNEL,
			INFO_SAMPLE_RATE,
			INFO_AVG_BYTE_PER_SEC,
			INFO_BLOCK_ALIGN,
			INFO_BIT_PER_SAMPLE,

			INFO_POSITION,
			INFO_POSITION_SAMPLE,
			INFO_LENGTH,
			INFO_LENGTH_SAMPLE,
		};

		static void PtrDelete(SoundPlayer* p) {
			p->Stop();
			delete p;
		}
	protected:
		DirectSoundManager* manager_;
		std::wstring path_;
		size_t pathHash_;
		gstd::CriticalSection lock_;
		IDirectSoundBuffer8* pDirectSoundBuffer_;
		shared_ptr<gstd::FileReader> reader_;
		SoundDivision* division_;

		SoundFileFormat format_;

		WAVEFORMATEX formatWave_;
		bool bLoop_;//���[�v�L��
		double timeLoopStart_;//���[�v�J�n����
		double timeLoopEnd_;//���[�v�I������
		bool bPause_;

		bool bDelete_;//�폜�t���O
		bool bFadeDelete_;//�t�F�[�h�A�E�g��ɍ폜
		bool bAutoDelete_;//�����폜
		double rateVolume_;//���ʊ���(0-100)
		double rateVolumeFadePerSec_;//�t�F�[�h���̉��ʒቺ����
		
		bool flgUpdateStreamOffset_;
		size_t audioSizeTotal_;		//In bytes.

		virtual bool _CreateBuffer(shared_ptr<gstd::FileReader> reader) = 0;
		virtual void _SetSoundInfo();
		static LONG _GetVolumeAsDirectSoundDecibel(float rate);
	public:
		SoundPlayer();
		virtual ~SoundPlayer();

		SoundFileFormat GetFormat() { return format_; }

		std::wstring& GetPath() { return path_; }
		size_t GetPathHash() { return pathHash_; }
		gstd::CriticalSection& GetLock() { return lock_; }
		virtual void Restore() { pDirectSoundBuffer_->Restore(); }
		void SetSoundDivision(SoundDivision* div);
		void SetSoundDivision(int index);

		virtual bool Play();
		virtual bool Play(PlayStyle& style);
		virtual bool Stop();
		virtual bool IsPlaying();
		virtual bool Seek(double time) = 0;
		virtual bool Seek(int64_t sample) = 0;
		virtual bool SetVolumeRate(double rateVolume);
		virtual void ResetStreamForSeek() {}
		bool SetPanRate(double ratePan);
		double GetVolumeRate();
		void SetFade(double rateVolumeFadePerSec);
		void SetFadeDelete(double rateVolumeFadePerSec);
		void SetAutoDelete(bool bAuto = true) { bAutoDelete_ = bAuto; }
		double GetFadeVolumeRate();
		void Delete() { bDelete_ = true; }
		WAVEFORMATEX* GetWaveFormat() { return &formatWave_; }

		virtual DWORD GetCurrentPosition();
		DWORD GetTotalAudioSize() { return audioSizeTotal_; }

		void SetFrequency(DWORD freq);
	};
	class SoundPlayer::PlayStyle {
		bool bLoop_;
		double timeLoopStart_;
		double timeLoopEnd_;
		double timeStart_;
		bool bRestart_;
	public:
		PlayStyle();
		virtual ~PlayStyle();
		void SetLoopEnable(bool bLoop) { bLoop_ = bLoop; }
		bool IsLoopEnable() { return bLoop_; }
		void SetLoopStartTime(double time) { timeLoopStart_ = time; }
		double GetLoopStartTime() { return timeLoopStart_; }
		void SetLoopEndTime(double time) { timeLoopEnd_ = time; }
		double GetLoopEndTime() { return timeLoopEnd_; }
		void SetStartTime(double time) { timeStart_ = time; }
		double GetStartTime() { return timeStart_; }
		bool IsRestart() { return bRestart_; }
		void SetRestart(bool b) { bRestart_ = b; }
	};

	/**********************************************************
	//SoundStreamPlayer
	**********************************************************/
	class SoundStreamingPlayer : public SoundPlayer {
		class StreamingThread;
		friend StreamingThread;
	protected:
		HANDLE hEvent_[3];
		IDirectSoundNotify* pDirectSoundNotify_;//�C�x���g
		size_t sizeCopy_;
		StreamingThread* thread_;
		bool bStreaming_;
		bool bRequestStop_;//���[�v�������̃t���O�B������~����ƍŌ�̃o�b�t�@���Đ�����Ȃ����߁B

		size_t lastStreamCopyPos_[2];
		DWORD bufferPositionAtCopy_[2];

		void _CreateSoundEvent(WAVEFORMATEX& formatWave);
		virtual void _CopyStream(int indexCopy);
		virtual size_t _CopyBuffer(LPVOID pMem, DWORD dwSize) = 0;
		void _RequestStop() { bRequestStop_ = true; }
	public:
		SoundStreamingPlayer();
		virtual ~SoundStreamingPlayer();

		virtual void ResetStreamForSeek();

		virtual bool Play(PlayStyle& style);
		virtual bool Stop();
		virtual bool IsPlaying();

		virtual DWORD GetCurrentPosition();
		size_t* DbgGetStreamCopyPos() { return lastStreamCopyPos_; }

		size_t GetReaderRefCount() { return reader_ ? 0 : reader_.use_count(); }
	};
	class SoundStreamingPlayer::StreamingThread : public gstd::Thread, public gstd::InnerClass<SoundStreamingPlayer> {
	public:
		StreamingThread(SoundStreamingPlayer* player) { _SetOuter(player); }

		virtual void _Run();

		void Notify(size_t index);
	};

	/**********************************************************
	//SoundPlayerWave
	**********************************************************/
	class SoundPlayerWave : public SoundPlayer {
	protected:
		virtual bool _CreateBuffer(shared_ptr<gstd::FileReader> reader);
	public:
		SoundPlayerWave();
		virtual ~SoundPlayerWave();

		virtual bool Play(PlayStyle& style);
		virtual bool Stop();
		virtual bool IsPlaying();
		virtual bool Seek(double time);
		virtual bool Seek(int64_t sample);
	};

	/**********************************************************
	//SoundStreamingPlayerWave
	**********************************************************/
	class SoundStreamingPlayerWave : public SoundStreamingPlayer {
	protected:
		size_t posWaveStart_;
		size_t posWaveEnd_;
		virtual bool _CreateBuffer(shared_ptr<gstd::FileReader> reader);
		virtual size_t _CopyBuffer(LPVOID pMem, DWORD dwSize);
	public:
		SoundStreamingPlayerWave();
		virtual bool Seek(double time);
		virtual bool Seek(int64_t sample);
	};

	/**********************************************************
	//SoundStreamingPlayerOgg
	**********************************************************/
	class SoundStreamingPlayerOgg : public SoundStreamingPlayer {
	protected:
		OggVorbis_File fileOgg_;
		ov_callbacks oggCallBacks_;

		virtual bool _CreateBuffer(shared_ptr<gstd::FileReader> reader);
		virtual size_t _CopyBuffer(LPVOID pMem, DWORD dwSize);

		static size_t _ReadOgg(void* ptr, size_t size, size_t nmemb, void* source);
		static int _SeekOgg(void* source, ogg_int64_t offset, int whence);
		static int _CloseOgg(void* source);
		static long _TellOgg(void* source);
	public:
		SoundStreamingPlayerOgg();
		~SoundStreamingPlayerOgg();
		virtual bool Seek(double time);
		virtual bool Seek(int64_t sample);
	};

	/**********************************************************
	//SoundStreamingPlayerMp3
	**********************************************************/
	class SoundStreamingPlayerMp3 : public SoundStreamingPlayer {
	protected:
		MPEGLAYER3WAVEFORMAT formatMp3_;
		WAVEFORMATEX formatWave_;
		HACMSTREAM hAcmStream_;
		ACMSTREAMHEADER acmStreamHeader_;
		size_t posMp3DataStart_;
		size_t posMp3DataEnd_;
		DWORD waveDataSize_;
		double timeCurrent_;
		gstd::ref_count_ptr<gstd::ByteBuffer> bufDecode_;

		virtual bool _CreateBuffer(shared_ptr<gstd::FileReader> reader);
		virtual size_t _CopyBuffer(LPVOID pMem, DWORD dwSize);
		size_t _ReadAcmStream(char* pBuffer, size_t size);
	public:
		SoundStreamingPlayerMp3();
		~SoundStreamingPlayerMp3();
		virtual bool Seek(double time);
		virtual bool Seek(int64_t sample);
	};
}

#endif
