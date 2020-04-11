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

drop table if exists products;
create table products (
      id serial primary key
    , name varchar
    , price decimal(8,4)
);

drop table if exists entries;
create table entries (
      id serial primary key
    , order_id integer
    , product_id integer
    , unit_price decimal(8,4)
    , discount decimal(8,4)
    , quantity decimal(8,4)
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
    /* , ('ERNSH', 1, '1996-07-17 12:00:00') */
    /* , ('CENTC', 4, '1996-07-18 12:00:00') */
    /* , ('QTTIK', 4, '1996-07-19 12:00:00') */
    /* , ('QUEDE', 4, '1996-07-19 12:00:00') */
    /* , ('RATTC', 8, '1996-07-22 12:00:00') */
    /* , ('ERNSH', 9, '1996-07-23 12:00:00') */
    /* , ('FOLKO', 6, '1996-07-24 12:00:00') */
    /* , ('BLONP', 2, '1996-07-25 12:00:00') */
    /* , ('WARTH', 3, '1996-07-26 12:00:00') */
;

insert into products (name, price) values
      ('Foo', 2.50)
    , ('Bar', 16.00)
    , ('Spam', 0.99)
    , ('Baz', 0.19)
    , ('Chicken', 17.00)
    , ('Tofu', 6.19)
    , ('Spinach', 3.00)
    , ('Cheese', 23.90)
;

insert into entries (order_id, product_id, unit_price, discount, quantity) values
      (1, 1, 2.50, 0.0, 2.0)
    , (1, 2, 16.00, 0.0, 0.16)
    , (2, 3, 0.99, 0.0, 30.0)
    , (3, 5, 42.00, 0.0, 1.0)
    , (4, 4, 0.19, 0.0, 12.0)
    , (5, 4, 0.19, 0.0, 8.0)
    , (6, 1, 2.20, 0.20, 50.0)
    , (6, 4, 0.19, 0.0, 10.0)
    , (7, 7, 3.0, 0.0, 0.250)
    , (8, 8, 23.90, 0.0, 0.487)
    , (9, 2, 16.00, 0.0, 1.35)
    , (9, 8, 23.90, 0.0, 0.291)
    , (9, 1, 2.40, 0.1, 4.000)
    , (10, 7, 3.00, 0.0, 0.814)
    , (10, 5, 6.19, 0.0, 0.404)
;
