CREATE TABLE `test_table`
(
        `key`   VARCHAR(32) ,
        `value` VARCHAR(256)
) ;

CREATE UNIQUE INDEX `test_table_idx1` on `test_table` ( `key` ) ;
-- DROP INDEX test_table_idx1 ON test_table ;

-- SELECT * FROM test_table;

-- TRUNCATE TABLE test_table;

