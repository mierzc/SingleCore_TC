ALTER TABLE db_version CHANGE COLUMN required_s0080_10307_01_mangos_instance_template required_s0080_10307_02_mangos_scripted_areatrigger bit;

RENAME TABLE areatrigger_scripts TO scripted_areatrigger;