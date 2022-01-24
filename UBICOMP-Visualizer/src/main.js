import App from './App.svelte';

const app = new App({
	target: document.body,
	props: {
		name: 'UBICOMP Visualizer'
	}
});

export default app;