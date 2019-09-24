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

#include "nrf_log.h"
#include "nrf_error.h"

#include <Weave/DeviceLayer/WeaveDeviceLayer.h>
#include <schema/include/SimpleCommandTrait.h>
#include <CommandSender.h>

using namespace ::nl::Weave;
using namespace ::nl::Weave::DeviceLayer;
using namespace ::nl::Weave::Profiles::DataManagement_Current;
using namespace ::nl::Weave::TLV;
using namespace ::Schema::Nest::Test::Trait;

CommandSender sCommandSender;

WEAVE_ERROR CommandSender::Init(uint64_t targetNodeId)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    PlatformMgr().LockWeaveStack();

    mTargetBinding = ::nl::Weave::DeviceLayer::ExchangeMgr.NewBinding(HandleBindingEvent, this);
    VerifyOrExit(mTargetBinding != NULL, err = WEAVE_ERROR_NO_MEMORY);

    mTargetNodeId = targetNodeId;
    mCommandEC = NULL;
    mReqBuf = NULL;

exit:
    PlatformMgr().UnlockWeaveStack();
    return err;
}

void CommandSender::SendCommand(uint32_t value)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    PlatformMgr().LockWeaveStack();

    if (mCommandEC != NULL)
    {
        mCommandEC->Abort();
        mCommandEC = NULL;
    }

    if (mReqBuf != NULL)
    {
        PacketBuffer::Free(mReqBuf);
        mReqBuf = NULL;
    }

    mReqBuf = PacketBuffer::New();
    VerifyOrExit(mReqBuf != NULL, err = WEAVE_ERROR_NO_MEMORY);

    err = EncodeCommandRequest(mReqBuf, value);
    SuccessOrExit(err);

    SendQueuedCommand();

exit:
    if (err != WEAVE_NO_ERROR)
    {
        PacketBuffer::Free(mReqBuf);
        mReqBuf = NULL;
        NRF_LOG_INFO("Error sending SimpleCommandTrait::SimpleCommandRequest command: %s", ::nl::ErrorStr(err));
    }

    PlatformMgr().UnlockWeaveStack();
}

void CommandSender::SendQueuedCommand(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if (mTargetBinding->GetState() != Binding::kState_Ready)
    {
        if (!mTargetBinding->IsPreparing())
        {
            err = mTargetBinding->RequestPrepare();
        }

        ExitNow();
    }

    err = mTargetBinding->NewExchangeContext(mCommandEC);
    SuccessOrExit(err);
    mCommandEC->AppState = this;
    mCommandEC->OnAckRcvd = HandleWRMPAckRcvd;
    mCommandEC->OnSendError = HandleSendError;

    NRF_LOG_INFO("Sending SimpleCommandTrait::SimpleCommandRequest command to %016" PRIx64, mTargetNodeId);

    err = mCommandEC->SendMessage(::nl::Weave::Profiles::kWeaveProfile_WDM, kMsgType_OneWayCommand, mReqBuf);
    mReqBuf = NULL;
    SuccessOrExit(err);

exit:
    if (err != WEAVE_NO_ERROR)
    {
        if (mCommandEC != NULL)
        {
            mCommandEC->Abort();
            mCommandEC = NULL;
        }
        PacketBuffer::Free(mReqBuf);
        mReqBuf = NULL;
        NRF_LOG_INFO("Error sending SimpleCommandTrait::SimpleCommandRequest command: %s", ::nl::ErrorStr(err));
    }
}

WEAVE_ERROR CommandSender::EncodeCommandRequest(PacketBuffer * buf, uint32_t value)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    TLVWriter tlvWriter;
    TLVType container;

    tlvWriter.Init(buf);

    {
        err = tlvWriter.StartContainer(AnonymousTag, kTLVType_Structure, container);
        SuccessOrExit(err);

        {
            err = tlvWriter.StartContainer(ContextTag(CustomCommand::kCsTag_Path), kTLVType_Path, container);
            SuccessOrExit(err);

            {
                err = tlvWriter.StartContainer(ContextTag(Path::kCsTag_InstanceLocator), kTLVType_Structure, container);
                SuccessOrExit(err);

                err = tlvWriter.Put(ContextTag(Path::kCsTag_TraitProfileID), (uint32_t)SimpleCommandTrait::kWeaveProfileId);
                SuccessOrExit(err);

                err = tlvWriter.Put(ContextTag(Path::kCsTag_TraitInstanceID), (uint32_t)0);
                SuccessOrExit(err);

                err = tlvWriter.EndContainer(kTLVType_Path);
                SuccessOrExit(err);
            }

            err = tlvWriter.EndContainer(kTLVType_Structure);
            SuccessOrExit(err);
        }

        err = tlvWriter.Put(ContextTag(CustomCommand::kCsTag_CommandType), (uint32_t)SimpleCommandTrait::kSimpleCommandRequestId);
        SuccessOrExit(err);

        {
            err = tlvWriter.StartContainer(ContextTag(CustomCommand::kCsTag_Argument), kTLVType_Structure, container);
            SuccessOrExit(err);

            err = tlvWriter.Put(ContextTag(SimpleCommandTrait::kSimpleCommandRequestParameter_Value), value);
            SuccessOrExit(err);

            err = tlvWriter.EndContainer(kTLVType_Structure);
            SuccessOrExit(err);
        }

        err = tlvWriter.EndContainer(kTLVType_NotSpecified);
        SuccessOrExit(err);
    }

    err = tlvWriter.Finalize();
    SuccessOrExit(err);

exit:
    return err;
}

void CommandSender::HandleBindingEvent(void *apAppState, Binding::EventType aEvent, const Binding::InEventParam & aInParam, Binding::OutEventParam & aOutParam)
{
    CommandSender * self = (CommandSender *)apAppState;
    static const WRMPConfig wrmpConfig =
    {
        100,        // Initial Retransmit Timeout
        100,        // Active Retransmit Timeout
        100,        // ACK Piggyback Timeout
        4           // Max Restransmissions
    };

    switch (aEvent)
    {
    case Binding::kEvent_PrepareRequested:
        aOutParam.PrepareRequested.PrepareError = self->mTargetBinding->BeginConfiguration()
            .Target_NodeId(self->mTargetNodeId)
            .TargetAddress_WeaveFabric(kWeaveSubnetId_ThreadMesh)
            .Transport_UDP_WRM()
            .Transport_DefaultWRMPConfig(wrmpConfig)
            .Security_None()
            .PrepareBinding();
        break;

    case Binding::kEvent_BindingReady:
        if (self->mReqBuf != NULL)
        {
            self->SendQueuedCommand();
        }
        break;

    default:
        Binding::DefaultEventHandler(apAppState, aEvent, aInParam, aOutParam);
        break;
    }
}

void CommandSender::HandleSendError(::nl::Weave::ExchangeContext *ec, WEAVE_ERROR err, void *msgCtxt)
{
    CommandSender * self = (CommandSender *)ec->AppState;
    if (self->mCommandEC == ec)
    {
        self->mCommandEC = NULL;
    }
    ec->Abort();
}

void CommandSender::HandleWRMPAckRcvd(::nl::Weave::ExchangeContext *ec, void *msgCtxt)
{
    HandleSendError(ec, WEAVE_NO_ERROR, msgCtxt);
}
