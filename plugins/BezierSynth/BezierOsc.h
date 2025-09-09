#ifndef LMMS_BEZIER_OSC_H
#define LMMS_BEZIER_OSC_H


#include <cassert>
#include <fftw3.h>
#include <cstdlib>

#include "Engine.h"
#include "lmms_constants.h"
#include "lmms_math.h"
#include "lmmsconfig.h"
#include "AudioEngine.h"
#include "OscillatorConstants.h"
#include "OscillatorBezier.h"
#include "OscillatorBezierZ.h"
#include "SampleBuffer.h"
// #include "MemoryManager.h"

namespace lmms
{

class SampleBuffer;


class LMMS_EXPORT BezierOsc
{
	// MM_OPERATORS
public:
	enum class WaveAlgo
	{
		Sine,
		Noise,
		BezierZ,
		Sample
		// TODO, many more
	};
	constexpr static auto NumWaveAlgos = 4;

	enum class ModulationAlgo
	{
		SignalMix,           // normal
		AmplitudeModulation, // wobble
		FrequencyModulation  // freaky
	} ;
	constexpr static auto NumModulationAlgos = 3;

	BezierOsc(
			const WaveAlgo wave_algo,
			const ModulationAlgo mod_algo,
			const float &freq,
			const float &detuning_div_samplerate,
			const float &volume,
			BezierOsc * m_subOsc = nullptr,
			SampleBuffer * m_userWave = nullptr
			);

	virtual ~BezierOsc()
	{
		delete m_subOsc;
	}

	inline void setUserWave( const SampleBuffer * _wave )
	{
		m_userWave = _wave;
	}

	void update(sampleFrame* ab, const fpp_t frames, const ch_cnt_t chnl, bool modulator = false);

	static inline sample_t sinSample( const float _sample )
	{
		return sinf( _sample * F_2PI );
	}

	static inline sample_t noiseSample( const float )
	{
		return 1.0f - fast_rand() / FAST_RAND_MAX;
	}

	inline sample_t userWaveSample( const float _sample ) const
	{
		// TODO play the whole thing once only
		if (m_userWave != nullptr) {
			return m_userWave->userWaveSample( _sample );
		}
		return 0;
	}

	inline sample_t bezierSample( const float _sample ) const
	{
		return m_bezier->oscSample( _sample );
	}


private:
	// N.B. not a model Core/Oscillator can change model mid-note, we dont support that
	const WaveAlgo m_waveAlgo;
	const ModulationAlgo m_modulationAlgo;
	const float & m_freq;
	const float & m_detuning_div_samplerate;
	const float & m_volume;
	BezierOsc * m_subOsc;
	float m_phaseOffset;
	float m_phase;
	bool m_isModulator;
	OscillatorBezier * m_bezier;
	const SampleBuffer * m_userWave;

	void updateNoSub( sampleFrame * _ab, const fpp_t _frames, const ch_cnt_t _chnl );
	void updatePM( sampleFrame * _ab, const fpp_t _frames, const ch_cnt_t _chnl );
	void updateAM( sampleFrame * _ab, const fpp_t _frames, const ch_cnt_t _chnl );
	void updateMix( sampleFrame * _ab, const fpp_t _frames, const ch_cnt_t _chnl );
	void updateFM( sampleFrame * _ab, const fpp_t _frames, const ch_cnt_t _chnl );

	template<WaveAlgo W>
	void updateNoSub( sampleFrame * _ab, const fpp_t _frames, const ch_cnt_t _chnl );
	template<WaveAlgo W>
	void updateAM( sampleFrame * _ab, const fpp_t _frames, const ch_cnt_t _chnl );
	template<WaveAlgo W>
	void updateMix( sampleFrame * _ab, const fpp_t _frames, const ch_cnt_t _chnl );
	template<WaveAlgo W>
	void updateFM( sampleFrame * _ab, const fpp_t _frames, const ch_cnt_t _chnl );

	template<WaveAlgo W>
	inline sample_t getSample( const float _sample );

	float syncInit( sampleFrame * _ab, const fpp_t _frames,
							const ch_cnt_t _chnl );
	inline bool syncOk( float _osc_coeff );

	inline void recalcPhase();

} ;


} // namespace lmms


#endif // LMMS_BEZIER_OSC_H
