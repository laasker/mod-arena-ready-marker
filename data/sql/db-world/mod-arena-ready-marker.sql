DELETE FROM `gameobject_template` WHERE `entry` = 301337;
INSERT INTO `gameobject_template` (`entry`, `type`, `displayId`, `name`, `IconName`, `castBarCaption`, `unk1`, `size`, `Data0`, `Data1`, `Data2`, `Data3`, `Data4`, `Data5`, `Data6`, `Data7`, `Data8`, `Data9`, `Data10`, `Data11`, `Data12`, `Data13`, `Data14`, `Data15`, `Data16`, `Data17`, `Data18`, `Data19`, `Data20`, `Data21`, `Data22`, `Data23`, `AIName`, `ScriptName`, `VerifiedBuild`) VALUES
(301337, 10, 327, 'Ready Marker', 'PVP', '', '', 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '', 'go_arena_ready_marker', 0);

DELETE FROM `gameobject_template_addon` WHERE `entry` = 301337;
INSERT INTO `gameobject_template_addon` (`entry`, `faction`, `flags`, `mingold`, `maxgold`, `artkit0`, `artkit1`, `artkit2`, `artkit3`) VALUES
(301337, 0, 32, 0, 0, 0, 0, 0, 0);

DELETE FROM `gameobject_template_locale` WHERE `entry` = 301337;
INSERT INTO `gameobject_template_locale` (`entry`, `locale`, `name`, `castBarCaption`, `VerifiedBuild`) VALUES
(301337, 'deDE', 'Bereitschaftsmarkierung', '', 0);

DELETE FROM `module_string` WHERE `module` = 'mod-arena-ready-marker';
INSERT INTO `module_string` (`module`, `id`, `string`) VALUES
('mod-arena-ready-marker', 0, 'You are marked as ready {}/{}'),
('mod-arena-ready-marker', 1, '{} is marked as ready {}/{}'),
('mod-arena-ready-marker', 2, 'An opponent is marked as ready {}/{}');
