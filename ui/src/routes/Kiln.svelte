<script lang="ts">
  import { stored_logs, current_state } from "../lib/stores";
  import { onMount } from 'svelte';
  import {
      Icon,
      Button,
      Row,
      Col,
      Modal,
      Table,
  } from 'sveltestrap';

  // Stop current schedule
  let stop_modal_open = false;
  const toggleStopModalOpen = () => (stop_modal_open = !stop_modal_open);
  async function uploadSchedule() {
    const response = await fetch(import.meta.env.VITE_KILN_URL + "kiln/schedule", {
      method: 'DELETE',
      headers: {'Content-Type': 'application/json'}
    });
    toggleStopModalOpen();
    if (!response.ok) { console.log(response) };
  }

  function calculateRemainingTime() {

  }

  // uplot: https://github.com/leeoniya/uPlot/tree/master/docs
  let plotContainer;
  function redraw(uPlot) {
    let x: Array<number> = [Date.now()/1000 - $current_state.runtime];  // timestamps
    let y: Array<number> = [0];  // temperature
    for (let step of $current_state.schedule.steps) {
      // previous timestamp plus time to reach target in seconds
      let ramptime: number = x.at(-1) + (step[1]/step[0])*3600;
      y.push(step[1], step[1]);
      // add ramptime plus hold to array
      x.push(ramptime, ramptime + step[2]*60);
    }
    let data = [x, y];
    console.log(data);
    const opts = {
        width: 600,
        height: 300,
        series: [{label: "Hours"}, {label: "°C", stroke: "teal"}]
    };
    let plot = new uPlot(opts, data, plotContainer);

    plot.setCursor({
      // top: plot.valToPos($current_state.temperature, 'x'),
      // left: plot.valToPos(Date.now()/1000, 'y'),
    })
  }
  onMount(async () => {
    const uplotModule = await import('uplot');
    const uPlot = uplotModule.default;
    redraw(uPlot);
  })
</script>
<style lang=css>
tbody {
  line-height: 1;
  font-size: 0.8rem;

  /* https://github.com/esphome/esphome-webserver/blob/main/v2/esp-log.ts#L105 */
  & .v {
    color: #888888;
  }
  & .d {
    color: #00dddd;
  }
  & .c {
    color: magenta;
  }
  & .i {
    color: limegreen;
  }
  & .w {
    color: yellow;
  }
  & .e {
    color: red;
    font-weight: bold;
  }
}
</style>

<Row>
  <Col>
    <Modal body header="Confirm upload schedule" isOpen={stop_modal_open} toggle={toggleStopModalOpen}>
      Are you sure you want to stop the running <code>{$current_state.schedule.name}</code> schedule on the kiln?
      <Row class="mt-4">
      <Col>
        <Button color="danger" on:click={() => toggleStopModalOpen()}>Cancel</Button>
        <Button class="float-end" color="success" on:click={() => uploadSchedule()}>Confirm</Button>
      </Col>
      </Row>
    </Modal>

    {#if $current_state?.schedule?.name !== ""}
      <span class="h2 ps-2">{$current_state.schedule.name}</span><br>
      <Button class="float-end me-1" color="danger" on:click={() => toggleStopModalOpen()}><Icon name="stop" /></Button>
      <span class="ps-2">
      Current temperature: {Math.round($current_state.temperature)}<br>
      Current running step: {$current_state.step}<br>
      Current  {$current_state.schedule.steps}<br>
      Remaining time in the schedule: <br>
      </span>
      {#key $current_state.temperature}
        <div bind:this={plotContainer}></div>
      {/key}
    {:else if $current_state?.schedule === null}
      <span class="h2 ps-2">No schedule running</span><br>
      <span class="ps-2">
      Current temperature: {Math.round($current_state.temperature)}<br>
      </span>
    {:else}
      <span class="h2 ps-2">Not connected to kiln</span><br>
    {/if}
  </Col>
  <Col>
    <Table borderless size="sm">
      <thead>
        <tr>
          <th>Time</th>
          <th>Level</th>
          <th>Tag</th>
          <th>Message</th>
        </tr>
      </thead>
      <tbody>
      {#each $stored_logs as log}
      <tr>
        <td class="{log.type} font-monospace">{log.when}</td>
        <td class="{log.type} font-monospace">{log.level}</td>
        <td class="{log.type} font-monospace">{log.type}</td>
        <td class="{log.type} font-monospace">{log.detail}</td>
      </tr>
      {/each}
      </tbody>
    </Table>
  </Col>
</Row>
