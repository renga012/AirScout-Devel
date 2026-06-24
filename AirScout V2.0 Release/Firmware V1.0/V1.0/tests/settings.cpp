    int32_t num = 0;
    char *str;
    bool booool = 0;
    double doob = 0.0;
    SETSTATUS ret;
    ret = user_settings.getint("last_update_unix", &num);
    printf("Reading num: %d with status %d\n", num, ret);
    ret = user_settings.getint("hardware/meas_interval_sec", &num);
    printf("Reading num: %d with status %d\n", num, ret);

    ret = user_settings.getString("hostname", &str);
    printf("Reading str: %s with status %d\n", str, ret);
    free(str);
    ret = user_settings.getString("language", &str);
    printf("Reading str: %s with status %d\n", str, ret);
    free(str);


    ret = user_settings.getBool("accounts/local_only", &booool);
    printf("Reading bool: %d with status %d\n", booool, ret);
    ret = user_settings.getBool("hardware/en_display", &booool);
    printf("Reading bool: %d with status %d\n", booool, ret);

    ret = user_settings.getDouble("last_update_unix", &doob);
    printf("Reading double: %f with status %d\n", doob, ret);
    
    user_settings.printjson();
    ret = user_settings.setint("maow", 20);
    ret = user_settings.setint("maow", 3);
    // printf("Writing Int: %d with status %d\n", 20, ret);
    // ret = user_settings.setint("1/2/3/4", 50);
    // ret = user_settings.setint("1/4", 50);
    // ret = user_settings.setint("1/6", 50);
    ret = user_settings.setint("1/2/3/4", 10);
    ret = user_settings.setint("1/2/3/4", 20);
    ret = user_settings.setint("1/2/3/4", 30);
    ret = user_settings.setint("1/2/2/aaa", 230);

    ret = user_settings.setBool("1/2/3/bool1", true);
    ret = user_settings.setBool("1/2/3/bool0", false);
    
    ret = user_settings.setDouble("1/3/double", 3487.23);
    ret = user_settings.setDouble("1/3/double1", 34817.2322);

    ret = user_settings.setString("1/3/srrrrrr", "Hello World11");
    ret = user_settings.setString("1/3/trrrtrreeee", "Hello Worldasdsad");

    ret = user_settings.setint("1/32/2/32/332/213/32", 50);
    ret = user_settings.setint("1/32/2/32/332/213/32", 500);
    ret = user_settings.setint("1/32/2/32/332/213/32", 5000);
    printf("Writing Int: %d with status %d\n", 50, ret);
    user_settings.printjson();
    sleep_ms(100);