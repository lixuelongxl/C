###################################################################
#           Copyright (c) 2016 by TAOS Technologies, Inc.
#                     All rights reserved.
#
#  This file is proprietary and confidential to TAOS Technologies.
#  No part of this file may be reproduced, stored, transmitted,
#  disclosed or used in any form or by any means other than as
#  expressly provided by the written permission from Jianhui Tao
#
###################################################################

# -*- coding: utf-8 -*-

import sys
import taos
from util.log import *
from util.cases import *
from util.sql import *
import numpy as np


class TDTestCase:
    def init(self, conn, logSql):
        tdLog.debug("start to execute %s" % __file__)
        tdSql.init(conn.cursor())

        self.rowNum = 10
        self.ts = 1537146000000
        
    def run(self):
        tdSql.prepare()

        tdSql.execute('''create table test(ts timestamp, col1 tinyint, col2 smallint, col3 int, col4 bigint, col5 float, col6 double, 
                    col7 bool, col8 binary(20), col9 nchar(20), col11 tinyint unsigned, col12 smallint unsigned, col13 int unsigned, col14 bigint unsigned) tags(loc nchar(20))''')
        tdSql.execute("create table test1 using test tags('beijing')")
        for i in range(self.rowNum):
            tdSql.execute("insert into test1 values(%d, %d, %d, %d, %d, %f, %f, %d, 'taosdata%d', '涛思数据%d', %d, %d, %d, %d)" 
                        % (self.ts + i, i + 1, i + 1, i + 1, i + 1, i + 0.1, i + 0.1, i % 2, i + 1, i + 1, i + 1, i + 1, i + 1, i + 1))
        
        # Count verifacation
        tdSql.query("select count(*) from test")        
        tdSql.checkData(0, 0, 10)

        tdSql.query("select count(ts) from test")        
        tdSql.checkData(0, 0, 10)                        
        tdSql.query("select count(col1) from test")        
        tdSql.checkData(0, 0, 10)
        tdSql.query("select count(col2) from test")        
        tdSql.checkData(0, 0, 10)
        tdSql.query("select count(col3) from test")        
        tdSql.checkData(0, 0, 10)
        tdSql.query("select count(col4) from test")        
        tdSql.checkData(0, 0, 10)
        tdSql.query("select count(col5) from test")        
        tdSql.checkData(0, 0, 10)
        tdSql.query("select count(col6) from test")        
        tdSql.checkData(0, 0, 10)
        tdSql.query("select count(col7) from test")        
        tdSql.checkData(0, 0, 10)
        tdSql.query("select count(col8) from test")        
        tdSql.checkData(0, 0, 10)
        tdSql.query("select count(col9) from test")        
        tdSql.checkData(0, 0, 10)

        tdSql.query("select count(col11) from test")        
        tdSql.checkData(0, 0, 10)
        tdSql.query("select count(col12) from test")        
        tdSql.checkData(0, 0, 10)
        tdSql.query("select count(col13) from test")        
        tdSql.checkData(0, 0, 10)
        tdSql.query("select count(col14) from test")        
        tdSql.checkData(0, 0, 10)

        tdSql.execute("alter table test add column col10 int")
        tdSql.query("select count(col10) from test")        
        tdSql.checkRows(0)
        
        #TS-653 forbidden count(tbname) to mix up with agg, proj etc.
        tdSql.error("select count(*),count(tbname) from test")
        tdSql.error("select col11,count(tbname) from test;")
        tdSql.error("select count(tbname),count(*) from test")
        tdSql.error("select count(tbname),col11 from test;")
        func_list=['avg','count','twa','sum','stddev','leastsquares','min',
                    'max','first','last','top','bottom','percentile','apercentile',
                    'last_row','diff','spread'
        ]
        for j in func_list:
            if j == 'leastsquares':
                pick_func=j+'(col1,1,1)'
            elif j == 'top' or j == 'bottom' or j == 'percentile' or j == 'apercentile':
                pick_func=j+'(col1,1)'
            else:
                pick_func=j+'(col)'
            tdSql.error("select {0},count(tbname) from test".format(pick_func))
            tdSql.error("select count(tbname),{0} from test".format(pick_func))

        tdSql.execute("insert into test1 values(now, 1, 2, 3, 4, 1.1, 2.2, false, 'test', 'test' , 1, 1, 1, 1, 1)")
        tdSql.query("select count(col10) from test")        
        tdSql.checkData(0, 0, 1)        
              
    def stop(self):
        tdSql.close()
        tdLog.success("%s successfully executed" % __file__)


tdCases.addWindows(__file__, TDTestCase())
tdCases.addLinux(__file__, TDTestCase())
