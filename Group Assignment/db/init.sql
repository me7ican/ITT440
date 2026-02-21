CREATE DATABASE IF NOT EXISTS itt440;
USE itt440;

CREATE TABLE IF NOT EXISTS scoreboard (
  user VARCHAR(50) NOT NULL,
  points INT NOT NULL DEFAULT 0,
  datetime_stamp DATETIME NOT NULL,
  PRIMARY KEY (user)
);

INSERT INTO scoreboard (user, points, datetime_stamp)
VALUES
('c_user', 0, NOW()),
('py_user', 0, NOW())
ON DUPLICATE KEY UPDATE datetime_stamp=VALUES(datetime_stamp);
