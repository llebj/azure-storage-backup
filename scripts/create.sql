CREATE TABLE IF NOT EXISTS profiles (
	id INTEGER PRIMARY KEY,
	fingerprint INTEGER NOT NULL,
	version INTEGER NOT NULL,
	name TEXT NOT NULL,
	source TEXT NOT NULL,
	destination TEXT NOT NULL,
	trigger_type INTEGER NOT NULL,
	status INTEGER NOT NULL,
	UNIQUE (fingerprint, version)
) STRICT;
