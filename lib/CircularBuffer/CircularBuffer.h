/*
    CircularBuffer - An Arduino circular buffering library for arbitrary types.

    Created by Ivo Pullens, Emmission, 2014 -- www.emmission.nl

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef CircularBuffer_h
#define CircularBuffer_h

#include <Arduino.h>

#ifdef ESP8266
#define DISABLE_IRQ noInterrupts()
#define RESTORE_IRQ interrupts()
#elif ESP32
#define DISABLE_IRQ ;
#define RESTORE_IRQ ;
#else
#define DISABLE_IRQ       \
    uint8_t sreg = SREG;    \
    cli();

#define RESTORE_IRQ        \
    SREG = sreg;
#endif

template <class BUFFERTYPE, uint8_t BUFFERSIZE>
class CircularBuffer {

    typedef BUFFERTYPE BufferType;
    BufferType Buffer[BUFFERSIZE];

    public:
        CircularBuffer() : m_buff(Buffer) {
            m_size = BUFFERSIZE;
            clear();
        }

        /** Clear all entries in the circular buffer. */
        void clear(void)
        {
            m_front = 0;
            m_fill  = 0;
        }

        /** Test if the circular buffer is empty */
        inline bool empty(void) const
        {
            return !m_fill;
        }

        /** Return the number of records stored in the buffer */
        inline uint8_t available(void) const
        {
            return m_fill;
        }

        /** Test if the circular buffer is full */
        inline bool full(void) const
        {
            return m_fill == m_size;
        }

        inline uint8_t getFill(void) const {
            return m_fill;
        }

        /** Aquire record on front of the buffer, for writing.
         * After filling the record, it has to be pushed to actually
         * add it to the buffer.
         * @return Pointer to record, or NULL when buffer is full.
         */
        BUFFERTYPE* getFront(void) const
        {
            DISABLE_IRQ;
            BUFFERTYPE* f = NULL;
            if (!full())
                f = get(m_front);
            RESTORE_IRQ;
            return f;
        }

        /** Push record to front of the buffer
         * @param record   Record to push. If record was aquired previously (using getFront) its
         *                 data will not be copied as it is already present in the buffer.
         * @return True, when record was pushed successfully.
         */
        bool pushFront(BUFFERTYPE* record)
        {
            bool ok = false;
            DISABLE_IRQ;
            if (!full())
            {
                BUFFERTYPE* f = get(m_front);
                if (f != record)
                    *f = *record;
                m_front = (m_front+1) % m_size;
                m_fill++;
                ok = true;
            }
            RESTORE_IRQ;
            return ok;
        }

        /** Aquire record on back of the buffer, for reading.
         * After reading the record, it has to be pop'ed to actually
         * remove it from the buffer.
         * @return Pointer to record, or NULL when buffer is empty.
         */
        BUFFERTYPE* getBack(void) const
        {
            BUFFERTYPE* b = NULL;
            DISABLE_IRQ;
            if (!empty())
                b = get(back());
            RESTORE_IRQ;
            return b;
        }

        /** Remove record from back of the buffer.
         * @return True, when record was pop'ed successfully.
         */
        bool popBack(void)
        {
            bool ok = false;
            DISABLE_IRQ;
            if (!empty())
            {
                m_fill--;
                ok = true;
            }
            RESTORE_IRQ;
            return ok;
        }

    protected:
        inline BUFFERTYPE * get(const uint8_t idx) const
        {
            return &(m_buff[idx]);
        }
        inline uint8_t back(void) const
        {
            return (m_front - m_fill + m_size) % m_size;
        }

        uint8_t          m_size;     // Total number of records that can be stored in the buffer.
        BUFFERTYPE* const m_buff;
        volatile uint8_t m_front;    // Index of front element (not pushed yet).
        volatile uint8_t m_fill;     // Amount of records currently pushed.
};

#endif // CircularBuffer_h