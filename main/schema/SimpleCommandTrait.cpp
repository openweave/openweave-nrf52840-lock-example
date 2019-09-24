/*
 *
 *    Copyright (c) 2019 Google LLC.
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

#include <schema/include/SimpleCommandTrait.h>

namespace Schema {
namespace Nest {
namespace Test {
namespace Trait {
namespace SimpleCommandTrait {

using namespace ::nl::Weave::Profiles::DataManagement;

//
// Property Table
//

const TraitSchemaEngine::PropertyInfo PropertyMap[] = {
};

//
// IsDictionary Table
//

uint8_t IsDictionaryTypeHandleBitfield[] = {
        0x0
};

//
// IsOptional Table
//

uint8_t IsOptionalHandleBitfield[] = {
        0x0
};

//
// IsNullable Table
//

uint8_t IsNullableHandleBitfield[] = {
        0x0
};

//
// IsEphemeral Table
//

uint8_t IsEphemeralHandleBitfield[] = {
        0x0
};

//
// Schema
//

const TraitSchemaEngine TraitSchema = {
    {
        kWeaveProfileId,
        PropertyMap,
        sizeof(PropertyMap) / sizeof(PropertyMap[0]),
        3,
#if (TDM_EXTENSION_SUPPORT) || (TDM_VERSIONING_SUPPORT)
        2,
#endif
        IsDictionaryTypeHandleBitfield,
        &IsOptionalHandleBitfield[0],
        NULL,
        &IsNullableHandleBitfield[0],
        &IsEphemeralHandleBitfield[0],
#if (TDM_EXTENSION_SUPPORT)
        NULL,
#endif
#if (TDM_VERSIONING_SUPPORT)
        NULL,
#endif
    }
};


} // namespace BoltLockTrait
} // namespace Security
} // namespace Trait
} // namespace Weave
} // namespace Schema
