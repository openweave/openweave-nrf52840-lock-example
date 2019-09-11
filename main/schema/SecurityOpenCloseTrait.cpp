
/**
 *    Copyright (c) 2019 Nest Labs, Inc.
 *    All rights reserved.
 *
 *    THIS FILE IS GENERATED. DO NOT MODIFY.
 *
 *    SOURCE TEMPLATE: trait.cpp
 *    SOURCE PROTO: nest/trait/security/security_open_close_trait.proto
 *
 */

#include <nest/trait/security/SecurityOpenCloseTrait.h>

namespace Schema {
namespace Nest {
namespace Trait {
namespace Security {
namespace SecurityOpenCloseTrait {

using namespace ::nl::Weave::Profiles::DataManagement;

//
// Property Table
//

const TraitSchemaEngine::PropertyInfo PropertyMap[] = {
    { kPropertyHandle_Root, 1 }, // open_close_state
    { kPropertyHandle_Root, 2 }, // first_observed_at
    { kPropertyHandle_Root, 3 }, // first_observed_at_ms
    { kPropertyHandle_Root, 32 }, // bypass_requested
};

//
// IsOptional Table
//

uint8_t IsOptionalHandleBitfield[] = {
        0x6
};

//
// IsNullable Table
//

uint8_t IsNullableHandleBitfield[] = {
        0x6
};

//
// IsEphemeral Table
//

uint8_t IsEphemeralHandleBitfield[] = {
        0x6
};

//
// Supported version
//
const ConstSchemaVersionRange traitVersion = { .mMinVersion = 1, .mMaxVersion = 2 };

//
// Schema
//

const TraitSchemaEngine TraitSchema = {
    {
        kWeaveProfileId,
        PropertyMap,
        sizeof(PropertyMap) / sizeof(PropertyMap[0]),
        1,
#if (TDM_EXTENSION_SUPPORT) || (TDM_VERSIONING_SUPPORT)
        5,
#endif
        NULL,
        &IsOptionalHandleBitfield[0],
        NULL,
        &IsNullableHandleBitfield[0],
        &IsEphemeralHandleBitfield[0],
#if (TDM_EXTENSION_SUPPORT)
        &Nest::Trait::Detector::OpenCloseTrait::TraitSchema,
#endif
#if (TDM_VERSIONING_SUPPORT)
        &traitVersion,
#endif
    }
};

    //
    // Events
    //

const nl::FieldDescriptor SecurityOpenCloseEventFieldDescriptors[] =
{
    {
        NULL, offsetof(SecurityOpenCloseEvent, openCloseState), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt32, 0), 1
    },

    {
        NULL, offsetof(SecurityOpenCloseEvent, priorOpenCloseState), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt32, 0), 2
    },

    {
        NULL, offsetof(SecurityOpenCloseEvent, bypassRequested), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeBoolean, 0), 32
    },

};

const nl::SchemaFieldDescriptor SecurityOpenCloseEvent::FieldSchema =
{
    .mNumFieldDescriptorElements = sizeof(SecurityOpenCloseEventFieldDescriptors)/sizeof(SecurityOpenCloseEventFieldDescriptors[0]),
    .mFields = SecurityOpenCloseEventFieldDescriptors,
    .mSize = sizeof(SecurityOpenCloseEvent)
};
const nl::Weave::Profiles::DataManagement::EventSchema SecurityOpenCloseEvent::Schema =
{
    .mProfileId = kWeaveProfileId,
    .mStructureType = 0x1,
    .mImportance = nl::Weave::Profiles::DataManagement::ProductionCritical,
    .mDataSchemaVersion = 2,
    .mMinCompatibleDataSchemaVersion = 1,
};

} // namespace SecurityOpenCloseTrait
} // namespace Security
} // namespace Trait
} // namespace Nest
} // namespace Schema
