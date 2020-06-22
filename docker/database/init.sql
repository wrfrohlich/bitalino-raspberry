CREATE DATABASE bitalino;
\c bitalino

CREATE TABLE raw_data (
	id serial not NULL,
	name VARCHAR(20) not NULL,
	ecg DECIMAL not NULL,
	eda DECIMAL not NULL,
	time VARCHAR(20) not NULL
);

CREATE TABLE processed (
	id serial not NULL,
	name VARCHAR(20) not NULL,
	ecg FLOAT not NULL,
	eda FLOAT not NULL,
	start_time VARCHAR(20) not NULL,
	final_time VARCHAR(20) not NULL
);

CREATE TABLE ecg (
	id serial not NULL,
	name VARCHAR(20) not NULL,
	heart_rate FLOAT not NULL,
	heart_rate_ts FLOAT not NULL
);