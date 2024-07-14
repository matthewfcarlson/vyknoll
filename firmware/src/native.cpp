// PlatformIO is stupid and can't disable a platform from building
// So we just build something stupid so we can use it as a testing platform

#ifdef NATIVE
int main() {
    return 0;
}
#endif