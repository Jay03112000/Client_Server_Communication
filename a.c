Access time Function :-

int compare_access_time(const void *a, const void *b) {
    const char *dir1 = *(const char **)a;
    const char *dir2 = *(const char **)b;

    struct stat stat1, stat2;
    if (stat(dir1, &stat1) != 0 || stat(dir2, &stat2) != 0) {
        // Error handling for stat() failures
        perror("stat");
        exit(EXIT_FAILURE);
    }

    // Compare file access times
    if (stat1.st_atime < stat2.st_atime) {
        return -1;  // dir1 comes before dir2
    } else if (stat1.st_atime > stat2.st_atime) {
        return 1;   // dir1 comes after dir2
    } else {
        return 0;   // Both file access times are equal
    }
}