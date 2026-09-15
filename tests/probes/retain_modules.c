/* Keep module storage alive across reloads in the lifecycle comparison process. */
int dlclose( void *handle ) {
	(void)handle;
	return 0;
}
