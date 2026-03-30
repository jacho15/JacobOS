void main() {
    //VGA start addr
    volatile char *video = (volatile char *) 0xB8000;

    video[0] = 'H';
    video[1] = 0x0F;
    video[2] = 'I';
    video[3] = 0x0F;
}