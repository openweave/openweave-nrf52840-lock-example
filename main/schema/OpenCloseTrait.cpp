
/**
 *    Copyright (c) 2019 Nest Labs, Inc.
 *    All rights reserved.
 *
 *    THIS FILE IS GENERATED. DO NOT MODIFY.
 *
 *    SOURCE TEMPLATE: trait.cpp
 *    SOURCE PROTO: nest/trait/detector/open_close_trait.proto
 *
 */

#include <nest/trait/detector/OpenCloseTrait.h>

namespace Schema {
namespace Nest {
namespace Trait {
namespace Detector {
namespace OpenCloseTrait {

using namespace ::nl::Weave::Profiles::DataManagement;

//
// Property Table
//

const TraitSchemaEngine::PropertyInfo PropertyMap[] = {
    { kPropertyHandle_Root, 1 }, // open_close_state
    { kPropertyHandle_Root, 2 }, // first_observed_at
    { kPropertyHandle_Root, 3 }, // first_observed_at_ms
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
        2,
#endif
        NULL,
        &IsOptionalHandleBitfield[0],
        NULL,
        &IsNullableHandleBitfield[0],
        &IsEphemeralHandleBitfield[0],
#if (TDM_EXTENSION_SUPPORT)
        NULL,
#endif
#if (TDM_VERSIONING_SUPPORT)
        &traitVersion,
#endif
    }
};

    //
    // Events
    //

const nl::FieldDescriptor OpenCloseEventFieldDescriptors[] =
{
    {
        NULL, offsetof(OpenCloseEvent, openCloseState), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt32, 0), 1
    },

    {
        NULL, offsetof(OpenCloseEvent, priorOpenCloseState), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt32, 0), 2
    },

};

const nl::SchemaFieldDescriptor OpenCloseEvent::FieldSchema =
{
    .mNumFieldDescriptorElements = sizeof(OpenCloseEventFieldDescriptors)/sizeof(OpenCloseEventFieldDescriptors[0]),
    .mFields = OpenCloseEventFieldDescriptors,
    .mSize = sizeof(OpenCloseEvent)
};
const nl::Weave::Profiles::DataManagement::EventSchema OpenCloseEvent::Schema =
{
    .mProfileId = kWeaveProfileId,
    .mStructureType = 0x1,
    .mImportance = nl::Weave::Profiles::DataManagement::Production,
    .mDataSchemaVersion = 2,
    .mMinCompatibleDataSchemaVersion = 1,
};

} // namespace OpenCloseTrait
} // namespace Detector
} // namespace Trait
} // namespace Nest
} // namespace Schema
