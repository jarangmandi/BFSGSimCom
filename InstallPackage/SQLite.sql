.mode csv
drop table if exists airportfrequencies;
drop table if exists airports;
.import airports.csv t_airports
.import airport-frequencies.csv t_airportfrequencies
create table airportfrequencies as select cast(id as bigint) as id, cast(airport_ref as bigint) as airport_ref, airport_ident, type, cast(frequency_mhz * 1000 as int) as frequency from t_airportfrequencies;
create index iairportfrequencies_airportref on airportfrequencies(airport_ref);
create table airports as select cast (id as bigint) as id, ident, name, cast(latitude_deg as double) as latitude, cast(longitude_deg as double) as longitude from t_airports;
create index iairports_id on airports(id);
drop table if exists t_airports;
drop table if exists t_airportfrequencies;
vacuum;

