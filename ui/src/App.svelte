<script lang="ts">
  import Router from 'svelte-spa-router';
  import {link} from 'svelte-spa-router';
  import active from 'svelte-spa-router/active'
  import routes from './routes';
  import { stored_logs, current_state, type RecordConfig} from "./lib/stores";

  import {
    Icon,
    Styles,
    Collapse,
    Navbar,
    NavbarToggler,
    NavbarBrand,
    Nav,
    NavItem,
    NavLink
  } from 'sveltestrap';

  let theme: string = 'auto';
  let nav_open: boolean = false;
  function navUpdate(event) {
    nav_open = event.detail.isOpen;
  }

  // start gathering logs
  // stock esphome
  const esphome = new EventSource(import.meta.env.VITE_KILN_URL + "events");
  const kiln_api = new EventSource(import.meta.env.VITE_KILN_URL + "kiln/state");
  // https://esphome.io/web-api/#event-source-api
  esphome.addEventListener("log", (e: MessageEvent) => {
    // https://github.com/esphome/esphome-webserver/blob/main/v2/esp-log.ts#L21
    const d: String = e.data;
    let parts = d.slice(10, d.length - 4).split(":");
    let tag = parts.slice(0, 2).join(":");
    let detail = d.slice(12 + tag.length, d.length - 4);
    const types: Record<string, string> = {
      "[1;31m": "e",
      "[0;33m": "w",
      "[0;32m": "i",
      "[0;35m": "c",
      "[0;36m": "d",
      "[0;37m": "v",
    };
    const record = {
      type: types[d.slice(0, 7)],
      level: d.slice(7, 10),
      tag: tag,
      detail: detail,
      when: new Date().toTimeString().split(" ")[0],
    } as RecordConfig;

    $stored_logs.unshift(record);
    $stored_logs = $stored_logs;
  });
  kiln_api.addEventListener("state", (e: MessageEvent) => {
    console.log(e.data);
    $current_state = JSON.parse(e.data);
  });
</script>

<main style="height: 100vh">
  <Styles {theme} />
  <Navbar color="primary-subtle mb-4" expand="md">
    <NavbarBrand href="/">Kiln Controller</NavbarBrand>
    <NavbarToggler on:click={() => (nav_open = !nav_open)} />
    <Collapse isOpen={nav_open} navbar expand="md" on:update={navUpdate}>
      <Nav class="ms-auto" navbar>
        <NavItem>
          <!-- use plain href to utilize use:active from spa-router -->
          <a href="/" class="nav-link" use:link use:active={'/'}>Schedules</a>
        </NavItem>
        <NavItem>
          <a href="/kiln" class="nav-link" use:link use:active={'/kiln'}>Kiln</a>
        </NavItem>
        <NavItem>
          <NavLink href="https://github.com/wilmardo/kiln-controller"><Icon name="github" /></NavLink>
        </NavItem>
      </Nav>
    </Collapse>
  </Navbar>

  <Router {routes}/>
</main>
