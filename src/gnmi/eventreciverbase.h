/*
 * Copyright (c) 2019 PANTHEON.tech.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef EVENTRECIVERBASE_H
#define EVENTRECIVERBASE_H

#include <exception>
#include <iostream>

class EventReceiverBase
{
public:
    virtual ~EventReceiverBase() {}
};

template<typename T>
class EventReceiver : public virtual EventReceiverBase
{
public:
    virtual void receiveWriteEvent(T *psender) = 0;
};

class EventSender
{
public:

    template<typename T>
    void registerReciver(T *receiver) {
        pReceiver = receiver;
    }

    void registerStream(void *stream) {
        this->stream = stream;
    }

    void *getStream() {
        return stream;
    }

    template<typename T>
    void sendEvent(T* pSender) {
        if (nullptr == pReceiver) {
            throw std::runtime_error("Receiver is not register.");
        }

        EventReceiver<T> *pCastedReceiver =
                                    dynamic_cast<EventReceiver<T>*>(pReceiver);
        pCastedReceiver->receiveWriteEvent(pSender);
    }

private:
    EventReceiverBase *pReceiver = nullptr;
    void *stream = nullptr;
};

template<typename T>
class BaseSender : public virtual EventSender
{
public:
    void send() {
        sendEvent((T*) this);
    }
};

#endif // EVENTRECIVERBASE_H
