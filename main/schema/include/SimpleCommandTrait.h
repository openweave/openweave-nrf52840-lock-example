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

#ifndef SIMPLE_COMMAND_TRAIT_H
#define SIMPLE_COMMAND_TRAIT_H

#include <Weave/Profiles/data-management/DataManagement.h>
#include <Weave/Support/SerializationUtils.h>



namespace Schema {
namespace Nest {
namespace Test {
namespace Trait {
namespace SimpleCommandTrait {

extern const nl::Weave::Profiles::DataManagement::TraitSchemaEngine TraitSchema;

enum {
      kWeaveProfileId = (0x235aU << 16) | 0xfeeeU
};

//
// Properties
//

enum {
    kPropertyHandle_Root = 1,
    //
    // Enum for last handle
    //
    kLastSchemaHandle = 1,
};

//
// Commands
//

enum {
    kSimpleCommandRequestId = 0x1,
};

enum SimpleCommandRequestParameters {
    kSimpleCommandRequestParameter_Value = 1,
};


} // namespace SimpleCommandTrait
} // namespace Trait
} // namespace Test
} // namespace Nest
} // namespace Schema
#endif // SIMPLE_COMMAND_TRAIT_H
