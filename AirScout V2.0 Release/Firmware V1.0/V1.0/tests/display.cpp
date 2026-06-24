/*
while(1) {
        START_PROFILE();
        disp.draw_hline(100, 30, 50, Display::COLOR::PURPLE);
        disp.draw_vline(200, 30, 50, Display::COLOR::BROWN);
        disp.draw_line(20, 20, 40, 80, Display::COLOR::CORNFLOWERBLUE);
        disp.draw_rectangle(200, 50, 60, 100, Display::COLOR::HOTPINK);
        disp.draw_circle(100, 400, 20, Display::COLOR::AQUAMARINE);

        disp.fill_circle(200, 400, 30, Display::COLOR::CRIMSON);
        disp.fill_rectangle(10, 10, 20, 40, Display::COLOR::CYAN);
        disp.fill_rectangle(40, 10, 50, 20, Display::COLOR::PINK);
        disp.draw_sprite_from_file("sprites/stuff/kitty_smol.bin", 20, 200);

        disp.draw_text(10, 380, 1, "Hello World!", &font_Liberation_Mono8x13, Display::COLOR::RED, Display::COLOR::WHITESMOKE);
        disp.draw_text(10, 405, 1, "Hello World!", &font_Liberation_Mono13x21, Display::COLOR::BLUE, Display::COLOR::YELLOW);
        disp.draw_text(0, 430, 1, "57% 22\260C", &font_Liberation_Mono29x47, Display::COLOR::GREY, Display::COLOR::GREEN);

        disp.fill_round_rectangle(40, 100, 240, 100, 20, Display::COLOR::LAWNGREEN);

        // disp.fill_round_rectangle(100, 200, 160, 100, 30, Display::COLOR::LAWNGREEN);
        END_PROFILE("Display Test")
        break;
    }
*/