#ifndef __GSTD_FPSCONTROLLER__
#define __GSTD_FPSCONTROLLER__

#include "../pch.h"

#include "SmartPointer.hpp"

namespace gstd {
	class FpsControlObject;
	/**********************************************************
	//FpsController
	**********************************************************/
	class FpsController {
	protected:
		DWORD fps_;			//�ݒ肳��Ă���FPS
		bool bUseTimer_;	//�^�C�}�[����
		bool bCriticalFrame_;
		bool bFastMode_;

		size_t fastModeFpsRate_;

		std::list<ref_count_weak_ptr<FpsControlObject>> listFpsControlObject_;

		inline DWORD _GetTime();
		inline void _Sleep(DWORD msec);
	public:
		FpsController();
		virtual ~FpsController();
		virtual void SetFps(DWORD fps) { fps_ = fps; }
		virtual DWORD GetFps() { return fps_; }
		virtual void SetTimerEnable(bool b) { bUseTimer_ = b; }

		virtual void Wait() = 0;
		virtual bool IsSkip() { return false; }
		virtual void SetCriticalFrame() { bCriticalFrame_ = true; }
		virtual float GetCurrentFps() = 0;
		virtual float GetCurrentWorkFps() { return GetCurrentFps(); }
		virtual float GetCurrentRenderFps() { return GetCurrentFps(); }
		bool IsFastMode() { return bFastMode_; }
		void SetFastMode(bool b) { bFastMode_ = b; }

		void SetFastModeRate(size_t fpsRate) { fastModeFpsRate_ = fpsRate; }

		void AddFpsControlObject(ref_count_weak_ptr<FpsControlObject> obj) {
			listFpsControlObject_.push_back(obj);
		}
		void RemoveFpsControlObject(ref_count_weak_ptr<FpsControlObject> obj);
		DWORD GetControlObjectFps();
	};

	/**********************************************************
	//StaticFpsController
	**********************************************************/
	class StaticFpsController : public FpsController {
	protected:
		float fpsCurrent_;		//���݂�FPS
		DWORD timePrevious_;			//�O��Wait�����Ƃ��̎���
		int timeError_;				//�����z������(�덷)
		DWORD timeCurrentFpsUpdate_;	//1�b�𑪒肷�邽�߂̎��ԕێ�
		size_t rateSkip_;		//�`��X�L�b�v��
		size_t countSkip_;		//�`��X�L�b�v�J�E���g
		std::list<DWORD> listFps_;	//1�b���ƂɌ���fps���v�Z���邽�߂�fps��ێ�
	public:
		StaticFpsController();
		~StaticFpsController();

		virtual void Wait();
		virtual bool IsSkip();
		virtual void SetCriticalFrame() { bCriticalFrame_ = true; timeError_ = 0; countSkip_ = 0; }

		void SetSkipRate(size_t value) {
			rateSkip_ = value;
			countSkip_ = 0;
		}
		virtual float GetCurrentFps() { return (fpsCurrent_ / (rateSkip_ + 1)); }
		virtual float GetCurrentWorkFps() { return fpsCurrent_; }
		virtual float GetCurrentRenderFps() { return GetCurrentFps(); }
	};

	/**********************************************************
	//AutoSkipFpsController
	**********************************************************/
	class AutoSkipFpsController : public FpsController {
	protected:
		float fpsCurrentWork_;		//���ۂ�fps
		float fpsCurrentRender_;	//���ۂ�fps
		DWORD timePrevious_;			//�O��Wait�����Ƃ��̎���
		DWORD timePreviousWork_;
		DWORD timePreviousRender_;
		int timeError_;				//�����z������(�덷)
		DWORD timeCurrentFpsUpdate_;	//1�b�𑪒肷�邽�߂̎��ԕێ�
		std::list<DWORD> listFpsWork_;
		std::list<DWORD> listFpsRender_;
		int countSkip_;			//�A���`��X�L�b�v��
		DWORD countSkipMax_;		//�ő�A���`��X�L�b�v��
	public:
		AutoSkipFpsController();
		~AutoSkipFpsController();

		virtual void Wait();
		virtual bool IsSkip() { return countSkip_ > 0; }
		virtual void SetCriticalFrame() { bCriticalFrame_ = true; timeError_ = 0; countSkip_ = 0; }

		virtual float GetCurrentFps() { return GetCurrentWorkFps(); }
		float GetCurrentWorkFps() { return fpsCurrentWork_; };
		float GetCurrentRenderFps() { return fpsCurrentRender_; };
	};

	/**********************************************************
	//FpsControlObject
	**********************************************************/
	class FpsControlObject {
	public:
		FpsControlObject() {}
		virtual ~FpsControlObject() {}
		virtual DWORD GetFps() = 0;
	};
}

#endif
