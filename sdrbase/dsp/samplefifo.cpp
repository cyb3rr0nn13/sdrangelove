///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2012 maintech GmbH, Otto-Hahn-Str. 15, 97204 Hoechberg, Germany //
// written by Christian Daniel                                                   //
//                                                                               //
// This program is free software; you can redistribute it and/or modify          //
// it under the terms of the GNU General Public License as published by          //
// the Free Software Foundation as version 3 of the License, or                  //
//                                                                               //
// This program is distributed in the hope that it will be useful,               //
// but WITHOUT ANY WARRANTY; without even the implied warranty of                //
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                  //
// GNU General Public License V3 for more details.                               //
//                                                                               //
// You should have received a copy of the GNU General Public License             //
// along with this program. If not, see <http://www.gnu.org/licenses/>.          //
///////////////////////////////////////////////////////////////////////////////////

#include "dsp/samplefifo.h"

#define MIN(x, y) (((x) < (y)) ? (x) : (y))

void SampleFifo::create(uint s)
{
	m_size = 0;
	m_fill = 0;
	m_head = 0;
	m_tail = 0;

	m_data.resize(s);
	m_size = m_data.size();

	if(m_size != s)
		qCritical("SampleFifo: out of memory");
}

SampleFifo::SampleFifo(QObject* parent) :
	QObject(parent),
	m_data()
{
	m_suppressed = -1;
	m_size = 0;
	m_fill = 0;
	m_head = 0;
	m_tail = 0;
}

SampleFifo::SampleFifo(int size, QObject* parent) :
	QObject(parent),
	m_data()
{
	m_suppressed = -1;

	create(size);
}

SampleFifo::~SampleFifo()
{
	QMutexLocker mutexLocker(&m_mutex);

	m_size = 0;
}

bool SampleFifo::setSize(int size)
{
	create(size);

	return m_size == (uint)size;
}

uint SampleFifo::write(const quint8* data, uint count)
{
	return write(SampleVector::const_iterator((Sample*)data), SampleVector::const_iterator((Sample*)(data + count)));
#if 0
	QMutexLocker mutexLocker(&m_mutex);
	uint total;
	uint remaining;
	uint len;
	const Sample* begin = (const Sample*)data;
	count /= sizeof(Sample);

	//qDebug("write pre count %d %u", count, m_fill);

	total = MIN(count, m_size - m_fill);
	if(total < count) {
		if(m_suppressed < 0) {
			m_suppressed = 0;
			m_msgRateTimer.start();
			qCritical("SampleFifo: overflow - dropping %u samples", count - total);
		} else {
			if(m_msgRateTimer.elapsed() > 2500) {
				qCritical("SampleFifo: %u messages dropped", m_suppressed);
				qCritical("SampleFifo: overflow - dropping %u samples", count - total);
				m_suppressed = -1;
			} else {
				m_suppressed++;
			}
		}
	}

	remaining = total;
	while(remaining > 0) {
		len = MIN(remaining, m_size - m_tail);
		//qDebug("write remaining %u, len %u", remaining, len);
		std::copy(begin, begin + len, m_data.begin() + m_tail);
		m_tail += len;
		m_tail %= m_size;
		m_fill += len;
		begin += len;
		remaining -= len;
	}

	//qDebug("write post count %d %u [%u;%u]", count, m_fill, m_head, m_tail);

	if(m_fill > 0)
		emit dataReady();

	return total;
#endif
}

uint SampleFifo::write(SampleVector::const_iterator begin, SampleVector::const_iterator end)
{
	QMutexLocker mutexLocker(&m_mutex);
	uint count = end - begin;
	uint total;
	uint remaining;
	uint len;

	total = MIN(count, m_size - m_fill);
	if(total < count) {
		if(m_suppressed < 0) {
			m_suppressed = 0;
			m_msgRateTimer.start();
			qCritical("SampleFifo: overflow - dropping %u samples", count - total);
		} else {
			if(m_msgRateTimer.elapsed() > 2500) {
				qCritical("SampleFifo: %u messages dropped", m_suppressed);
				qCritical("SampleFifo: overflow - dropping %u samples", count - total);
				m_suppressed = -1;
			} else {
				m_suppressed++;
			}
		}
	}

	remaining = total;
	while(remaining > 0) {
		len = MIN(remaining, m_size - m_tail);
		std::copy(begin, begin + len, m_data.begin() + m_tail);
		m_tail = (m_tail + len) % m_size;
		m_fill += len;
		begin += len;
		remaining -= len;
	}

	if(m_fill > 0)
		emit dataReady();

	return total;
}
/*
uint SampleFifo::read(SampleVector::iterator begin, SampleVector::iterator end)
{
	QMutexLocker mutexLocker(&m_mutex);
	uint count = end - begin;
	uint total;
	uint remaining;
	uint len;

	total = MIN(count, m_fill);
	if(total < count)
		qCritical("SampleFifo: underflow - missing %u samples", count - total);

	remaining = total;
	while(remaining > 0) {
		len = MIN(remaining, m_size - m_head);
		std::copy(m_data.begin() + m_head, m_data.begin() + m_head + len, begin);
		m_head += len;
		m_head %= m_size;
		m_fill -= len;
		begin += len;
		remaining -= len;
	}

	return total;
}
*/
uint SampleFifo::readBegin(uint count,
	SampleVector::iterator* part1Begin, SampleVector::iterator* part1End,
	SampleVector::iterator* part2Begin, SampleVector::iterator* part2End)
{
	QMutexLocker mutexLocker(&m_mutex);
	uint total;
	uint done = 0;
	uint remaining;
	uint len;
	uint head = m_head;

	total = MIN(count, m_fill);
	if(total < count)
		qCritical("SampleFifo: underflow - missing %u samples", count - total);

	remaining = total;
	if(remaining > 0) {
		len = MIN(remaining, m_size - head);
		*part1Begin = m_data.begin() + head;
		*part1End = *part1Begin + len;
		head = (head + len) % m_size;
		remaining -= len;
		done += len;
	} else {
		*part1Begin = m_data.end();
		*part1End = m_data.end();
	}
	if(remaining > 0) {
		len = MIN(remaining, m_size - head);
		*part2Begin = m_data.begin() + head;
		*part2End = *part2Begin + len;
		done += len;
	} else {
		*part2Begin = m_data.end();
		*part2End = m_data.end();
	}

	return done;
}

uint SampleFifo::readCommit(uint count)
{
	QMutexLocker mutexLocker(&m_mutex);

	if(count > m_fill) {
		qCritical("SampleFifo: cannot commit more than available samples");
		count = m_fill;
	}
	m_head = (m_head + count) % m_size;
	m_fill -= count;

	return count;
}
