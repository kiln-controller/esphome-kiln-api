<script lang="ts">
  import { stored_schedules } from "../lib/stores";
  import "uplot/dist/uPlot.min.css";
  import { Input, Table, Button, Icon, Modal, Row, Col } from "sveltestrap";

  // autofocus for input fields: https://svelte.dev/repl/aac847deb62844f6ac31716c0354b09f?version=3.42.1
  async function autofocus(e) {
    await tick();
    e.focus();
  }

  let selected_schedule: string = "";

  // schedule modal
  let new_schedule_name: string = "";
  let schedule_name_modal_open: boolean = false;
  let schedule_name_model_action: string = "";
  function toggleScheduleNameModalOpen(action: string) {
    schedule_name_modal_open = !schedule_name_modal_open;
    schedule_name_model_action = action;
  }
  function scheduleNameModal(e: Event) {
    if (e instanceof KeyboardEvent && e.key !== "Enter") return;
    switch (schedule_name_model_action) {
      case "add": {
        addSchedule(new_schedule_name);
        break;
      }
      case "copy": {
        copySchedule(selected_schedule, new_schedule_name);
        break;
      }
      case "move": {
        moveSchedule(selected_schedule, new_schedule_name);
        break;
      }
    }
    toggleScheduleNameModalOpen("");
  }

  // Schedule CRUD
  function addSchedule(schedule: string) {
    $stored_schedules[schedule] = [[0, 0, 0]];
    selected_schedule = schedule;
    new_schedule_name = "";
  }
  function deleteSchedule(schedule: string) {
    delete $stored_schedules[schedule];
    $stored_schedules = $stored_schedules;
    selected_schedule = "";
  }
  function copySchedule(schedule: string, new_name: string) {
    // https://stackoverflow.com/a/61498717
    $stored_schedules[new_name] = JSON.parse(
      JSON.stringify($stored_schedules[schedule])
    );
    selected_schedule = new_name;
    new_schedule_name = "";
  }
  function moveSchedule(schedule: string, new_name: string) {
    copySchedule(schedule, new_name);
    deleteSchedule(schedule);
    selected_schedule = new_name;
  }

  // upload selected schedule to the Kiln
  let upload_modal_open = false;
  const toggleUploadModalOpen = () => (upload_modal_open = !upload_modal_open);
  async function uploadSchedule() {
    const response = await fetch(
      import.meta.env.VITE_KILN_URL + "kiln/schedule",
      {
        method: "POST",
        body: JSON.stringify({
          name: selected_schedule,
          schedule: $stored_schedules[selected_schedule],
        }),
        headers: { "Content-Type": "application/json" },
      }
    );
    toggleUploadModalOpen();
    if (!response.ok) {
      console.log(response);
    }
  }

  // Step CRUD
  function addStep() {
    $stored_schedules[selected_schedule].push([0, 0, 0]);
    $stored_schedules = $stored_schedules;
  }
  function deleteStep(index: number) {
    $stored_schedules[selected_schedule].splice(index, 1);
    $stored_schedules = $stored_schedules;
  }

  // uplot: https://github.com/leeoniya/uPlot/tree/master/docs
  //   let plotContainer;
  //   function redraw() {
  //     // currently the previous schedule
  //     // let x: Array = [1, 2, 3, 4, 5];
  //     // let y: Array = [1, 3, 2, 5, 4];
  //     let x: Array<number> = [0];
  //     let y: Array<number> = [0];
  //     for (let step of $stored_schedules[selected_schedule]) {
  //       // previous timestamp plus time to reach target
  //       let ramptime: number = x.at(-1) + step[1]/step[0];
  //       y.push(step[1], step[1]);
  //       // add ramptime plus hold to array
  //       x.push(ramptime, ramptime + step[2]/60);
  //     }
  //     let data = [x, y];
  //     console.log(data);
  //     const opts = {
  //         width: 600,
  //         height: 300,
  //         scales: {x: {time: false}},
  //         series: [{label: "Hours"}, {label: "°C", stroke: "red"}],
  //     };

  //     new uPlot(opts, data, plotContainer);
  //   }
</script>

<div class="input-group mb-4 px-2">
  <Input type="select" name="scheduleSelect" bind:value={selected_schedule}>
    <option value="" disabled selected hidden>Select a schedule</option>
    {#each Object.entries($stored_schedules) as [name, schedule]}
      <option value={name}>{name}</option>
    {/each}
  </Input>
  <Button color="success" on:click={() => toggleScheduleNameModalOpen("add")}
    ><Icon name="plus-lg" /></Button
  >
</div>

<Modal
  body
  header="Schedule name"
  isOpen={schedule_name_modal_open}
  toggle={toggleScheduleNameModalOpen}
>
  <div class="input-group">
    <Input
      required
      name="scheduleName"
      placeholder="Schedule name"
      bind:value={new_schedule_name}
      on:keydown={(e) => scheduleNameModal(e)}
    />
    <Button color="primary" on:click={(e) => scheduleNameModal(e)}
      ><Icon name="send" /></Button
    >
  </div>
</Modal>

{#if selected_schedule}
  <span class="h2 ps-2">{selected_schedule}</span>
  <Button
    class="float-end me-2"
    color="danger"
    on:click={() => deleteSchedule(selected_schedule)}
    ><Icon name="trash" /></Button
  >
  <Button
    class="float-end me-1"
    on:click={() => toggleScheduleNameModalOpen("move")}
    ><Icon name="pencil" /></Button
  >
  <!--copy icon seems missing in sveltestrap -->
  <Button
    class="float-end me-1"
    on:click={() => toggleScheduleNameModalOpen("copy")}
    ><Icon name="subtract" /></Button
  >
  <Button
    class="float-end me-1"
    color="warning"
    on:click={() => toggleUploadModalOpen()}><Icon name="upload" /></Button
  >

  <Modal
    body
    header="Confirm upload schedule"
    isOpen={upload_modal_open}
    toggle={toggleUploadModalOpen}
  >
    Are you sure you want to start schedule <code>{selected_schedule}</code> on
    the kiln?
    <Row class="mt-4">
      <Col>
        <Button color="danger" on:click={() => toggleUploadModalOpen()}
          >Cancel</Button
        >
        <Button
          class="float-end"
          color="success"
          on:click={() => uploadSchedule()}>Confirm</Button
        >
      </Col>
    </Row>
  </Modal>

  <Table class="mt-4" hover>
    <thead>
      <tr>
        <th>Step</th>
        <th>Ramp in °C/h</th>
        <th>Target °C</th>
        <th colspan="2">Hold in minutes</th>
      </tr>
    </thead>
    <tbody>
      {#each $stored_schedules[selected_schedule] as schedule, index}
        <tr>
          <th scope="row">{index}</th>
          {#each schedule as step}
            <td><Input required type="number" bind:value={step} /></td>
          {/each}
          <td
            ><Button
              class="float-end"
              outline
              color="danger"
              on:click={() => deleteStep(index)}><Icon name="trash" /></Button
            ></td
          >
        </tr>
      {/each}
      <tr>
        <td colspan="5"
          ><Button block outline on:click={() => addStep()}
            ><Icon name="plus-lg" /></Button
          ></td
        >
      </tr>
    </tbody>
  </Table>
{/if}
<!-- <div bind:this={plotContainer}></div> -->
