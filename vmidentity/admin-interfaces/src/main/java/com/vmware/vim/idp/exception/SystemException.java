/*
 *  Copyright (c) 2012-2015 VMware, Inc.  All Rights Reserved.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may not
 *  use this file except in compliance with the License.  You may obtain a copy
 *  of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, without
 *  warranties or conditions of any kind, EITHER EXPRESS OR IMPLIED.  See the
 *  License for the specific language governing permissions and limitations
 *  under the License.
 */
package com.vmware.vim.idp.exception;

/**
 * Base exception indicating bad condition of the system which prevents normal
 * completion of the given request. System errors might be caused by hardware
 * failures, software defects (bugs), network problems or when the service is
 * down.
 */
public abstract class SystemException extends RuntimeException {

   private static final long serialVersionUID = -5334186756507932465L;

   public SystemException(String message, Throwable cause) {
      super(message, cause);
   }

   public SystemException(String message) {
      super(message);
   }

   public SystemException(Throwable cause) {
      super(cause);
   }

}
