﻿/*
 * Copyright © 2012-2015 VMware, Inc.  All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the “License”); you may not
 * use this file except in compliance with the License.  You may obtain a copy
 * of the License at http://www.apache.org/licenses/LICENSE-2.0
 *·
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an “AS IS” BASIS, without
 * warranties or conditions of any kind, EITHER EXPRESS OR IMPLIED.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 */
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.IO;

namespace VMwareMMCIDP.UI.Common.Utilities
{
    public static class MMCMiscUtil
    {
        public static void SaveDataToFile(string data, string dialogTitle, string dialogFilter)
        {
            using (var sfd = new SaveFileDialog())
            {
                sfd.Title = dialogTitle;
                sfd.Filter = dialogFilter;
                if (sfd.ShowDialog() == DialogResult.OK)
                {
                    File.WriteAllText(sfd.FileName, data);
                }
            }
        }
        public static string SelectFile(string dialogTitle, string dialogFilter)
        {
            using (var ofd = new OpenFileDialog())
            {
                ofd.Title = dialogTitle;
                ofd.Filter = dialogFilter;
                if (ofd.ShowDialog() == DialogResult.OK)
                {
                    return ofd.FileName;
                }
                return String.Empty;
            }
        }
    }
}
