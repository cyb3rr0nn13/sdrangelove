#ifndef INCLUDE_INTERPOLATOR_H
#define INCLUDE_INTERPOLATOR_H

#include "dsp/dsptypes.h"
#include "util/export.h"

class SDRANGELOVE_API Interpolator {
public:
	Interpolator();

	void create(int phaseSteps, double sampleRate, double cutoff);

	bool interpolate(Real* distance, const Complex& next, bool* consumed, Complex* result)
	{
		while(*distance >= 1.0) {
			if(!(*consumed)) {
				advanceFilter(next);
				*distance -= 1.0;
				*consumed = true;
			} else {
				return false;
			}
		}
		doInterpolate((int)floor(*distance * (Real)m_phaseSteps), result);
		return true;
	}


private:
	std::vector<Real> m_taps;
	std::vector<Complex> m_samples;
	int m_ptr;
	int m_phaseSteps;
	int m_nTaps;

	void createTaps(int nTaps, double sampleRate, double cutoff, std::vector<Real>* taps);

	void advanceFilter(const Complex& next)
	{
		m_ptr--;
		if(m_ptr < 0)
			m_ptr = m_nTaps - 1;
		m_samples[m_ptr] = next;
	}

	void doInterpolate(int phase, Complex* result)
	{
		int sample = m_ptr;
		const Real* coeff = &m_taps[phase * m_nTaps];
		Real rAcc = 0;
		Real iAcc = 0;

		for(int i = 0; i < m_nTaps; i++) {
			rAcc += *coeff * m_samples[sample].real();
			iAcc += *coeff * m_samples[sample].imag();
			sample = (sample + 1) % m_nTaps;
			coeff++;
		}
		*result = Complex(rAcc, iAcc);
	}
};

#endif // INCLUDE_INTERPOLATOR_H
