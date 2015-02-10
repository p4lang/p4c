################################################################
#
#        Copyright 2013, Big Switch Networks, Inc. 
#        Copyright 2013, Barefoot Networks, Inc. 
# 
# Licensed under the Eclipse Public License, Version 1.0 (the
# "License"); you may not use this file except in compliance
# with the License. You may obtain a copy of the License at
# 
#        http://www.eclipse.org/legal/epl-v10.html
# 
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
# either express or implied. See the License for the specific
# language governing permissions and limitations under the
# License.
#
################################################################

#
# The root of of our repository is here:
#
ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))

#
# Resolve submodule dependencies. 
#
ifndef SUBMODULE_INFRA
  ifdef SUBMODULES
    SUBMODULE_INFRA := $(SUBMODULES)/infra
  else
    SUBMODULE_INFRA := $(ROOT)/submodules/infra
    SUBMODULES_LOCAL += infra
  endif
endif

ifdef SUBMODULES_LOCAL
  SUBMODULES_LOCAL_UPDATE := $(shell python $(ROOT)/submodules/init.py --update $(SUBMODULES_LOCAL))
  ifneq ($(lastword $(SUBMODULES_LOCAL_UPDATE)),submodules:ok.)
    $(info Local submodule update failed.)
    $(info Result:)
    $(info $(SUBMODULES_LOCAL_UPDATE))
    $(error Abort)
  endif
endif

export SUBMODULE_INFRA
export BUILDER := $(SUBMODULE_INFRA)/builder/unix

MODULE_DIRS := $(ROOT)/modules $(SUBMODULE_INFRA)/modules

.show-submodules:
	@echo infra @ $(SUBMODULE_INFRA)














