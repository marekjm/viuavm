drop table if exists employees;
create table employees (
      id serial primary key
    , name varchar
);

drop table if exists orders;
create table orders (
      id serial primary key
    , customer varchar
    , employee int
    , order_date timestamp
);

insert into employees (name) values
      ('John Doe')
    , ('Jane Doe')
    , ('Julius Caesar')
    , ('Platformus Teknologicus')
    , ('Testowus Itemus')
    , ('Asterix')
    , ('Obelix')
    , ('Panoramix')
    , ('Idefix')
    , ('Ostatni')
;

insert into orders (customer, employee, order_date) values
      ('VINET', 5, '1996-07-04 12:00:00')
    , ('TOMSP', 6, '1996-07-05 12:00:00')
    , ('HANAR', 4, '1996-07-08 12:00:00')
    , ('VICTE', 3, '1996-07-08 12:00:00')
    , ('SUPRD', 4, '1996-07-09 12:00:00')
    , ('HANAR', 3, '1996-07-10 12:00:00')
    , ('CHOPS', 5, '1996-07-11 12:00:00')
    , ('RICSU', 9, '1996-07-12 12:00:00')
    , ('WELLI', 3, '1996-07-15 12:00:00')
    , ('HILAA', 4, '1996-07-16 12:00:00')
    , ('ERNSH', 1, '1996-07-17 12:00:00')
    , ('CENTC', 4, '1996-07-18 12:00:00')
    , ('QTTIK', 4, '1996-07-19 12:00:00')
    , ('QUEDE', 4, '1996-07-19 12:00:00')
    , ('RATTC', 8, '1996-07-22 12:00:00')
    , ('ERNSH', 9, '1996-07-23 12:00:00')
    , ('FOLKO', 6, '1996-07-24 12:00:00')
    , ('BLONP', 2, '1996-07-25 12:00:00')
    , ('WARTH', 3, '1996-07-26 12:00:00')
;

select * from employees;
select * from orders;
