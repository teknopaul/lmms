#ifndef LMMS_BEZIER_OSC_H
#define LMMS_BEZIER_OSC_H


#include <cassert>
#include <fftw3.h>
#include <cstdlib>
#include <QObject>

#include "Engine.h"
#include "lmms_constants.h"
#include "lmms_math.h"
#include "lmmsconfig.h"
#include "AudioEngine.h"
#include "AutomatableModel.h"
#include "OscillatorConstants.h"
#include "OscillatorBezier.h"
#include "OscillatorBezierZ.h"
#include "SampleBuffer.h"
// #include "MemoryManager.h"

namespace lmms
{

class SampleBuffer;

/**
 * @brief The BezierOsc class generates sound waves using one of a few different algorythms.
 * 4 of these are created for each note play they are chained together with m_subOsc.
 */
class LMMS_EXPORT BezierOsc : public QObject
{
	Q_OBJECT
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
			const float freq,
			const float detuning_div_samplerate,
			const float volume,
			FloatModel * mutateModel,
			const float attack,
			BezierOsc * m_subOsc = nullptr,
			SampleBuffer * m_userWave = nullptr
			);

	virtual ~BezierOsc()
	{
		delete m_subOsc;
	}

	inline void setUserWave( const SampleBuffer * wave )
	{
		m_userWave = wave;
	}

	// writes both channels
	void update(sampleFrame* ab, const fpp_t frames, bool clean);

	static inline sample_t sinSample( const float sample )
	{
		return sinf( sample * F_2PI );
	}

	static inline sample_t noiseSample( const float )
	{
		return (1.0f - fast_rand() * 2.0f / FAST_RAND_MAX) * 0.25f;
	}

	inline sample_t userWaveSample( const float sample ) const
	{
		// TODO play the whole thing once only
		if (m_userWave != nullptr) {
			return m_userWave->userWaveSample( sample );
		}
		return 0;
	}

	inline sample_t bezierSample( const float sample ) const
	{
		return m_bezier->oscSample( sample );
	}

public slots:
	void mutateChanged();

private:
	// N.B. not a model Core/Oscillator can change model mid-note, we dont support that
	const WaveAlgo m_waveAlgo;
	const ModulationAlgo m_modulationAlgo;
	const float m_freq;
	const float m_detuning_div_samplerate;
	const float m_volume;
	FloatModel * m_mutateModel;
	// duration in seconds of attack
	const float m_attack;
	BezierOsc * m_subOsc;
	float m_phaseOffset;
	float m_phase;
	sample_rate_t m_sample_rate;
	OscillatorBezier * m_bezier;
	using handleState = SampleBuffer::handleState;
	const SampleBuffer * m_userWave;
	uint32_t m_frames_played;

	void updateNoSub( sampleFrame * sampleArrays, const fpp_t frames, bool clean );
	void updateNoSubNoise( sampleFrame * sampleArrays, const fpp_t frames, bool clean );
	void updateAM( sampleFrame * sampleArrays, const fpp_t frames, bool clean );
	void updateMix( sampleFrame * sampleArrays, const fpp_t frames, bool clean );
	void updateMixNoise( sampleFrame * sampleArrays, const fpp_t frames, bool clean );
	void updateFM( sampleFrame * sampleArrays, const fpp_t frames, bool clean );

	template<WaveAlgo W>
	void updateNoSub( sampleFrame * sampleArrays, const fpp_t frames, bool clean );
	template<WaveAlgo W>
	void updateAM( sampleFrame * sampleArrays, const fpp_t frames , bool clean);
	template<WaveAlgo W>
	void updateMix( sampleFrame * sampleArrays, const fpp_t frames, bool clean );
	template<WaveAlgo W>
	void updateFM( sampleFrame * sampleArrays, const fpp_t frames, bool clean );

	template<WaveAlgo W>
	inline sample_t getSample( const float sample );

	float syncInit( sampleFrame * sampleArrays, const fpp_t frames, bool clean );
	inline bool syncOk( float osc_coeff );
	inline sample_t fadeOut(uint32_t frames_played, float seconds);
	inline sample_t fadeIn(uint32_t frames_played, float seconds);
	inline void recalcPhase();

} ;


} // namespace lmms


#endif // LMMS_BEZIER_OSC_H
