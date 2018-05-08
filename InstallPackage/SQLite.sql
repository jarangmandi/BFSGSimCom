.mode csv

drop table if exists airportfrequencies;
drop table if exists airports;

.import airports.csv t_airports
.import airport-frequencies.csv t_airportfrequencies

create table airportfrequencies as select cast(id as bigint) as id, cast(airport_ref as bigint) as airport_ref, airport_ident, type, cast(frequency_mhz * 100 as int) as frequency from t_airportfrequencies;
delete from airportfrequencies where type not in ('GND','CLD','RCO','CTAF','TWR','RDO','ATF','AWOS','AFIS','ATIS','APP','ARR','DEP','CNTR');
create index iairportfrequencies_airportref on airportfrequencies(airport_ref);

create table airports as select cast (id as bigint) as id, name, cast(latitude_deg as double) as latitude, cast(longitude_deg as double) as longitude from t_airports;
delete from airports where id not in (select distinct airport_ref from airportfrequencies);
create index iairports_id on airports(id);

drop table if exists t_airports;
drop table if exists t_airportfrequencies;

vacuum;

