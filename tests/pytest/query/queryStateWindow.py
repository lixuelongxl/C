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
        self.rowNum = 100000
        self.ts = 1537146000000
        
    def run(self):
        tdSql.prepare()

        print("==============step1")
        tdSql.execute(
            "create table if not exists st (ts timestamp, t1 int, t2 timestamp, t3 bigint, t4 float, t5 double, t6 binary(10), t7 smallint, t8 tinyint, t9 bool, t10 nchar(10), t11 int unsigned, t12 bigint unsigned, t13 smallint unsigned, t14 tinyint unsigned ,t15 int) tags(dev nchar(50), tag2 binary(16))")
        tdSql.execute(
            'CREATE TABLE if not exists dev_001 using st tags("dev_01", "tag_01")')
        tdSql.execute(
            'CREATE TABLE if not exists dev_002 using st tags("dev_02", "tag_02")') 

        print("==============step2")

        tdSql.execute(
            "INSERT INTO dev_001 VALUES('2020-05-13 10:00:00.000', 1, '2020-05-13 10:00:00.000', 10, 3.1, 3.14, 'test', -10, -126, true, '测试', 15, 10, 65534, 254, 1)('2020-05-13 10:00:01.000', 1, '2020-05-13 10:00:01.000', 10, 3.1, 3.14, 'test', -10, -126, true, '测试', 15, 10, 65534, 253, 5)('2020-05-13 10:00:02.000', 10, '2020-05-13 10:00:00.000', 11, 3.1, 3.14, 'test', 10, -127, false, '测试', 15, 10, 65534, 253, 10)('2020-05-13 10:00:03.000', 1, '2020-05-13 10:00:00.000', 11, 3.1, 3.14, 'test', -10, -126, true, '测试', 14, 12, 65532, 254, 15)")

        for i in range(10):
            sql = "insert into dev_002(ts, t1) values"
            batch = int(self.rowNum / 10)
            for j in range(batch):
                sql += "(%d, %d)" % (self.ts + batch * i + j, batch * i + j)
            tdSql.execute(sql)

        tdSql.query("select count(ts) from dev_001 state_window(t1)")
        tdSql.checkRows(3)
        tdSql.checkData(0, 0, 2)
        tdSql.query("select count(ts) from dev_001 state_window(t3)")
        tdSql.checkRows(2)
        tdSql.checkData(1, 0, 2)
        tdSql.query("select count(ts) from dev_001 state_window(t7)")
        tdSql.checkRows(3)
        tdSql.checkData(1, 0, 1)
        tdSql.query("select count(ts) from dev_001 state_window(t8)")
        tdSql.checkRows(3)
        tdSql.checkData(2, 0, 1)
        tdSql.query("select count(ts) from dev_001 state_window(t11)")
        tdSql.checkRows(2)
        tdSql.checkData(0, 0, 3)     
        tdSql.query("select count(ts) from dev_001 state_window(t12)")
        tdSql.checkRows(2)
        tdSql.checkData(1, 0, 1)     
        tdSql.query("select count(ts) from dev_001 state_window(t13)")
        tdSql.checkRows(2)
        tdSql.checkData(1, 0, 1)         
        tdSql.query("select count(ts) from dev_001 state_window(t14)")
        tdSql.checkRows(3)
        tdSql.checkData(1, 0, 2)
        tdSql.query("select count(ts) from dev_002 state_window(t1)")
        tdSql.checkRows(100000)
        
        # with all aggregate function
        tdSql.query("select count(*),sum(t1),avg(t1),twa(t1),stddev(t15),leastsquares(t15,1,1),first(t15),last(t15),spread(t15),percentile(t15,90),t9 from dev_001 state_window(t9);")
        tdSql.checkRows(3)
        tdSql.checkData(0, 0, 2)
        tdSql.checkData(1, 1, 10)
        tdSql.checkData(0, 2, 1)             
        # tdSql.checkData(0, 3, 1) 
        tdSql.checkData(0, 4, np.std([1,5])) 
        # tdSql.checkData(0, 5, 1) 
        tdSql.checkData(0, 6, 1)         
        tdSql.checkData(0, 7, 5)         
        tdSql.checkData(0, 8, 4)         
        tdSql.checkData(0, 9, 4.6)         
        tdSql.checkData(0, 10, 'True')         

        # with where
        tdSql.query("select avg(t15),t9 from dev_001 where  t9='true' state_window(t9);")
        tdSql.checkData(0, 0, 7)  
        tdSql.checkData(0, 1, 'True')  

        # error      
        tdSql.error("select count(*) from dev_001 state_window(t2)")
        tdSql.error("select count(*) from st state_window(t3)")
        tdSql.error("select count(*) from dev_001 state_window(t4)")
        tdSql.error("select count(*) from dev_001 state_window(t5)")
        tdSql.error("select count(*) from dev_001 state_window(t6)")
        tdSql.error("select count(*) from dev_001 state_window(t10)")
        tdSql.error("select count(*) from dev_001 state_window(tag2)")

        # TS-537
        tdLog.info("case for TS-537")
        tdSql.execute("create stable stb (ts timestamp, c1 int, c2 float) tags(tg1 int)")
        tdSql.execute("CREATE TABLE IF NOT EXISTS db.tb1 USING db.stb TAGS (1)")
        sql = "insert into tb1 values(1635398806734, 1, 1.000000)(1635398810062, 1, 2.000000)(1635398811528, 1, 3.000000)(1635398813301, 1, 4.000000)"
        sql += "(1635398818507, 2, 1.000000)(1635398823464, 2, 1.000000)(1635398825150, 2, 1.000000)(1635398826453, 2, 1.000000)(1635399123037, 2, 2.000000)"
        sql += "(1635399125335, 2, 2.000000)(1635399126292, 2, 2.000000)(1635399127288, 2, 2.000000)(1635399129361, 2, 2.000000)(1635399133331, 1, 2.000000)"
        sql += "(1635399134179, 1, 2.000000)(1635399134909, 1, 2.000000)(1635399135617, 1, 2.000000)(1635399136372, 1, 2.000000)"
        tdSql.execute(sql)

        tdSql.query("select * from (select first(ts), count(*), c1 from db.tb1 state_window(c1))")
        tdSql.checkRows(3)
        tdSql.checkData(0, 1, 4)
        tdSql.checkData(0, 2, 1)
        tdSql.checkData(1, 1, 9)
        tdSql.checkData(1, 2, 2)
        tdSql.checkData(2, 1, 5)
        tdSql.checkData(2, 2, 1)

        tdSql.query("select fts, cnt, c1 from (select first(ts) fts, count(*) cnt, c1 from db.tb1 state_window(c1))")
        tdSql.checkRows(3)
        tdSql.checkData(0, 1, 4)
        tdSql.checkData(0, 2, 1)
        tdSql.checkData(1, 1, 9)
        tdSql.checkData(1, 2, 2)
        tdSql.checkData(2, 1, 5)
        tdSql.checkData(2, 2, 1)

        tdSql.query("select * from (select first(ts) fts, count(*) cnt, c1 from db.tb1 state_window(c1))")
        tdSql.checkRows(3)
        tdSql.checkData(0, 1, 4)
        tdSql.checkData(0, 2, 1)
        tdSql.checkData(1, 1, 9)
        tdSql.checkData(1, 2, 2)
        tdSql.checkData(2, 1, 5)
        tdSql.checkData(2, 2, 1)

    def stop(self):
        tdSql.close()
        tdLog.success("%s successfully executed" % __file__)


tdCases.addWindows(__file__, TDTestCase())
tdCases.addLinux(__file__, TDTestCase())
