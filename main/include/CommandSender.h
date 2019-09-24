/*
 *
 *    Copyright (c) 2018 Nest Labs, Inc.
 *    All rights reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#ifndef COMMAND_SENDER_H
#define COMMAND_SENDER_H

#include <Weave/Profiles/data-management/TraitData.h>
#include <schema/include/SimpleCommandTrait.h>

class CommandSender
{
public:
    WEAVE_ERROR Init(uint64_t targetNodeId);

    void SendCommand(uint32_t value);

private:
    uint64_t mTargetNodeId;
    ::nl::Weave::Binding * mTargetBinding;
    ::nl::Weave::ExchangeContext * mCommandEC;
    ::nl::Weave::PacketBuffer * mReqBuf;

    void SendQueuedCommand(void);
    WEAVE_ERROR EncodeCommandRequest(::nl::Weave::PacketBuffer * buf, uint32_t value);

    static void HandleBindingEvent(void *apAppState, ::nl::Weave::Binding::EventType aEvent,
            const ::nl::Weave::Binding::InEventParam & aInParam, ::nl::Weave::Binding::OutEventParam & aOutParam);
    static void HandleSendError(::nl::Weave::ExchangeContext *ec, WEAVE_ERROR err, void *msgCtxt);
    static void HandleWRMPAckRcvd(::nl::Weave::ExchangeContext *ec, void *msgCtxt);
};

extern CommandSender sCommandSender;

#endif // COMMAND_SENDER_H
