счетчикНажатийЗапуска = 0;
запускБылНажат = 0;
пока (счетчикНажатийЗапуска < @@RUN_CLICKS@@) {
	если (не нажалиЗапуск() и запускБылНажат) {
		увеличить(счетчикНажатийЗапуска);
		запускБылНажат = 0;
	}
	если (нажалиЗапуск()) {
		запускБылНажат = 1;
	}
}